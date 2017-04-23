/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jhalford <jack@crans.org>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2017/04/22 14:10:24 by jhalford          #+#    #+#             */
/*   Updated: 2017/04/23 18:18:41 by jhalford         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

int g_pid=-1;

unsigned short checksum(void *b, int len)
{
	unsigned short	*buf = b;
	unsigned int	sum=0;
	unsigned short	result;

	for (sum = 0; len > 1; len -= 2)
		sum += *buf++;
	if (len == 1)
		sum += *(unsigned char*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return (result);
}

void	display(void *buf, int bytes, struct sockaddr_in *addr)
{
	int		i;
	struct ip	*ip = buf;
	struct icmp *icmp = buf + ip->ip_hl*4;

	printf("%d bytes from %s: ttl=%i\n",
			ip->ip_len, inet_ntoa(addr->sin_addr), ip->ip_ttl);
	printf("IPv%d: hrd-size=%d, pkt-size=%d, id=%d, frag-off=%d, protocol=%c\n, ttl=%i",
			ip->ip_v, ip->ip_hl, ip->ip_len, ip->ip_id, ip->ip_off, ip->ip_p, ip->ip_ttl);
	if (icmp->icmp_hun.ih_idseq.icd_id == g_pid)
	{
		printf("ICMP: type[%d/%d] checksum[%d] id[%d] seq[%d]\n",
				icmp->icmp_type, icmp->icmp_code, ntohs(icmp->icmp_cksum),
				icmp->icmp_hun.ih_idseq.icd_id, icmp->icmp_hun.ih_idseq.icd_seq);
		(void)bytes;
	}
	for (i=0; i < bytes; i++)
	{
		if ( !(i & 15) ) printf("\n%i: ", i);
		printf("%c ", ((char*)buf)[i]);
	}
	printf("\n");
}

void	listener(struct sockaddr_in *addr)
{
	int		sd;
	struct sockaddr_in	r_addr;
	int		bytes;
	socklen_t		len;
	unsigned char	buf[1024];
	
	if ((sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_ICMP)) < 0)
	{
		perror("socket");
		exit(0);
	}
	for (;;)
	{
		bzero(buf, sizeof(buf));
		len = sizeof(r_addr);
		bytes = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr*)&r_addr, &len);
		if (bytes > 0)
			display(buf, bytes, addr);
		else
			perror("recvfrom");
	}
}

void	ping(struct sockaddr_in *addr)
{
	const int		val;
	int				i;
	int				sd;
	int				cnt;
	socklen_t		len;
	struct s_packet	pkt;
	struct sockaddr_in	r_addr;

	if ((sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_ICMP)) < 0)
	{
		perror("socket");
		return ;
	}
	if (setsockopt(sd, 0, IP_TTL, &val, sizeof(val)) != 0)
		perror("set TTL option");
	if (fcntl(sd, F_SETFL, O_NONBLOCK) != 0)
		perror("Request non blocking IO");
	cnt = 1;
	while (1)
	{
		len = sizeof(r_addr);
		recvfrom(sd, &pkt, sizeof(pkt), 0, (struct sockaddr*)&r_addr, &len);
		bzero(&pkt, sizeof(pkt));
		pkt.icmp.icmp_type = ICMP_ECHO;
		pkt.icmp.icmp_hun.ih_idseq.icd_id = g_pid;
		for (i=0; i < (int)sizeof(pkt.msg); i++)
			pkt.msg[i] = i+'0';
		pkt.msg[i] = 0;
		pkt.icmp.icmp_hun.ih_idseq.icd_seq = cnt++;
		pkt.icmp.icmp_cksum = checksum(&pkt, sizeof(pkt));
		if (sendto(sd, &pkt, sizeof(pkt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0)
			perror("sendto");
		sleep(1);
	}
}

int		main(int ac, char **av)
{
	struct sockaddr_in	*addr;
	struct addrinfo		*result;

	if (ac != 2)
	{
		printf("usage: %s <addr>\n", av[0]);
		exit(1);
	}
	if (getaddrinfo(av[1], NULL, NULL, &result) != 0)
	{
		perror("getaddrinfo");
		exit(1);
	}
	addr = (struct sockaddr_in*)result->ai_addr;
	if (fork() == 0)
	{
		listener(addr);
		exit(0);
	}
	else
		ping(addr);
	wait(0);
	return (0);
}
