#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pcap.h>
#include <time.h>
#include <libgen.h>
#include <signal.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>  
#include <arpa/inet.h>
#include <sys/types.h>
#include <hiredis/hiredis.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define	OK	(0)
#define	ERR	(-1)
#define	PEND	(1)

#define	SIZE_IP	16
#define	SIZE_ETHERNET	14
#define ETHER_ADDR_LEN	6
#define HOSTNAME_LEN	128

#define	PKT_TYPE_TCP	1
#define	PKT_TYPE_UDP	2

#define	OUTPUT_INTERVAL	300

#define SLL_HDR_LEN     16              /* total header length */
#define SLL_ADDRLEN     8               /* length of address field */
         
struct sll_header {                                                                                                           
        u_int16_t       sll_pkttype;    /* packet type */
        u_int16_t       sll_hatype;     /* link-layer address type */
        u_int16_t       sll_halen;      /* link-layer address length */
        u_int8_t        sll_addr[SLL_ADDRLEN];  /* link-layer address */
        u_int16_t       sll_protocol;   /* protocol */
};       

/* Ethernet header */
struct sniff_ethernet {
	u_char ether_dhost[ETHER_ADDR_LEN]; /* Destination host address */
	u_char ether_shost[ETHER_ADDR_LEN]; /* Source host address */
	u_short ether_type; /* IP? ARP? RARP? etc */
};

/* IP header */
struct sniff_ip {
	u_char ip_vhl;		/* version << 4 | header length >> 2 */
	u_char ip_tos;		/* type of service */
	u_short ip_len;		/* total length */
	u_short ip_id;		/* identification */
	u_short ip_off;		/* fragment offset field */
#define IP_RF 0x8000		/* reserved fragment flag */
#define IP_DF 0x4000		/* dont fragment flag */
#define IP_MF 0x2000		/* more fragments flag */
#define IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
	u_char ip_ttl;		/* time to live */
	u_char ip_p;		/* protocol */
	u_short ip_sum;		/* checksum */
	struct in_addr ip_src,ip_dst; /* source and dest address */
};
#define IP_HL(ip)		(((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)		(((ip)->ip_vhl) >> 4)

/* TCP header */
struct sniff_tcp {
	uint16_t th_sport;	/* source port */
	uint16_t th_dport;	/* destination port */
	uint32_t th_seq;	/* sequence number */
	uint32_t th_ack;	/* acknowledgement number */

	u_char th_offx2;	/* data offset, rsvd */
#define TH_OFF(th)	(((th)->th_offx2 & 0xf0) >> 4)
	u_char th_flags;
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
	u_short th_win;		/* window */
	u_short th_sum;		/* checksum */
	u_short th_urp;		/* urgent pointer */
};

const struct sll_header *sllhdr; /* The ethernet header */
const struct sniff_ethernet *ethernet; /* The ethernet header */
const struct sniff_ip *iphdr; /* The IP header */
const struct sniff_tcp *tcphdr; /* The TCP header */
const char *payload; /* Packet payload */

u_int size_iphdr;
u_int size_tcphdr;

void my_callback(u_char* useless, const struct pcap_pkthdr* h,
const u_char* s) {

	printf("ssss");
	fflush(stdout);

        struct tm       *tm;
        time_t          timeStampSec; 
        char            src_ip[SIZE_IP], dst_ip[SIZE_IP];
        uint16_t        src_port, dst_port;
        
        timeStampSec = h->ts.tv_sec;
        tm = localtime(&timeStampSec);


        //ethernet = (struct sniff_ethernet*)(s);
	sllhdr = (struct sll_header*)(s);                                                                                     

	//iphdr = (struct sniff_ip *)(s + SIZE_ETHERNET);
	iphdr = (struct sniff_ip *)(s + SLL_HDR_LEN);

        size_iphdr = IP_HL(iphdr)*4;
        if (size_iphdr < 20) {
                printf("   * Invalid IP header length: %u bytes", size_iphdr);
                return; 
        }      
        
        tcphdr = (struct sniff_tcp *)(s + SLL_HDR_LEN + size_iphdr);
        size_tcphdr = TH_OFF(tcphdr)*4;
        if (size_tcphdr < 20) {
                printf("   * Invalid TCP header length: %u bytes\n", size_tcphdr);
                return;
        }      
        
        inet_ntop(AF_INET, (void *)&(iphdr->ip_src), src_ip, SIZE_IP);
        inet_ntop(AF_INET, (void *)&(iphdr->ip_dst), dst_ip, SIZE_IP);
        src_port = ntohs(tcphdr->th_sport);
        dst_port = ntohs(tcphdr->th_dport);
        
        //printf("%d-%d-%d %d:%d:%d caplen:%d len:%d %s:%d->%s:%d ip_total_bytes:%d\n\n", 
        printf("caplen:%d len:%d %s:%d->%s:%d ip_total_bytes:%d\n\n", 
                h->caplen, h->len,
                src_ip, src_port, dst_ip, dst_port,
                ntohs(iphdr->ip_len));
}      
       
int main(int argc, char** argv)
{      
        int i;
        char *dev;
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_t *descr;
        const u_char *packet;
        struct pcap_pkthdr hdr;
        struct ether_header *epthr;
        struct bpf_program fp;
        bpf_u_int32     maskp;
        bpf_u_int32     netp;
       
        dev = pcap_lookupdev(errbuf);

	printf("%s", dev);
	fflush(stdout);
        pcap_lookupnet(dev, &netp, &maskp, errbuf);
       
        descr = pcap_open_live(dev, 100, 0, -1, errbuf); //ok
        descr = pcap_open_live("any", 100, 0, -1, errbuf); //tcp header error
                                                                                                                              
        pcap_compile(descr, &fp, argv[1], 0, netp);
       
        pcap_setfilter(descr, &fp);
       
        pcap_loop(descr, -1, my_callback, "kkk");
       
        fprintf(stdout, "aoaoa");
        
        return 0;
}      
