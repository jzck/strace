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

#include "ping.h"

int 	g_pid=-1;
int		g_pkt_rec=0;
char	g_domain[256];

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
	struct ip	*ip = buf;
	struct icmp *icmp;
	struct s_packet	*pkt;
	int		hlen;
	char	addrstr[INET_ADDRSTRLEN];
	struct timeval	start, end, triptime;
	double	diff;

	(void)bytes;
	hlen = ip->ip_hl << 2;
	pkt = (struct s_packet*)(buf + hlen);
	icmp = &pkt->hdr;
	start = *(struct timeval*)&pkt->msg;

	if (gettimeofday(&end, NULL) != 0)
		return ;
	timersub(&end, &start, &triptime);
	diff = (triptime.tv_sec + triptime.tv_usec / 1000000.0) * 1000.0;
	rs_push(diff);
	g_pkt_rec++;
	printf("%d bytes from %s: icmp_seq=%d ttl=%i time=%.3f ms\n",
			ip->ip_len,
			inet_ntop(AF_INET, &(addr->sin_addr), addrstr, INET_ADDRSTRLEN),
			icmp->icmp_seq, ip->ip_ttl, diff);
}

void listener(void)
{	int sd;
	struct sockaddr_in addr;
	unsigned char buf[1024];

	sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_ICMP);
	if ( sd < 0 )
	{
		perror("socket");
		exit(0);
	}
	for (;;)
	{	int bytes;
		socklen_t len=sizeof(addr);

		bzero(buf, sizeof(buf));
		bytes = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr*)&addr, &len);
		if ( bytes > 0 )
			display(buf, bytes, &addr);
		else
			perror("recvfrom");
	}
	exit(0);
}

void	ping(struct sockaddr_in *addr)
{
	const int		val = 255;
	int				i;
	int				sd;
	int				cnt;
	socklen_t		len;
	struct s_packet	pkt;
	struct sockaddr_in	r_addr;
	struct timeval	time;

	if ((sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_ICMP)) < 0)
	{
		perror("socket");
		return ;
	}
	if (setsockopt(sd, 0, IP_TTL, &val, sizeof(val)) != 0)
		perror("set TTL option");
	if (fcntl(sd, F_SETFL, O_NONBLOCK) != 0)
		perror("Request non blocking IO");
	cnt = 0;
	while (1)
	{
		len = sizeof(r_addr);
		bzero(&pkt, sizeof(pkt));
		pkt.hdr.icmp_type = ICMP_ECHO;
		pkt.hdr.icmp_id = g_pid;
		pkt.hdr.icmp_seq = cnt++;

		for (i=0; i < (int)sizeof(pkt.msg); i++)
			pkt.msg[i] = i+'0';
		pkt.msg[i] = 0;
		if (gettimeofday(&time, NULL) != 0)
			return ;
		ft_memcpy(pkt.msg, (void*)&time, sizeof(time));
		time = *(struct timeval*)&pkt.msg;
		pkt.hdr.icmp_cksum = checksum(&pkt, sizeof(pkt));
		if (sendto(sd, &pkt, sizeof(pkt), 0, (struct sockaddr*)addr, sizeof(*addr)) <= 0)
			perror("sendto");
		sleep(1);
	}
}

void	sigint_handler(int signo)
{
	double		loss;

	(void)signo;
	rs_calcmore();
	loss = g_rs.count ? 100 * ((float) (g_rs.count - g_pkt_rec) / (float)g_rs.count) : 0;
	printf("\n--- %s ping statistics ---", g_domain);
	printf("\n%d packets transmitted, %d packets received, %0.1f%% packet loss", g_rs.count, g_pkt_rec, loss);
	printf("\nround-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms", g_rs.min, g_rs.avg, g_rs.max, g_rs.stdev);
	exit(0);
}

int		main(int ac, char **av)
{
	struct sockaddr_in	*addr;
	struct addrinfo		*result, hints;
	char				addrstr[INET_ADDRSTRLEN];

	if (ac != 2)
	{
		printf("usage: %s <addr>\n", av[0]);
		exit(1);
	}
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_CANONNAME;
	if (getaddrinfo(av[1], NULL, &hints, &result) != 0)
	{
		perror("getaddrinfo");
		exit(1);
	}
	addr = (struct sockaddr_in*)result->ai_addr;
	inet_ntop(AF_INET, &(addr->sin_addr), addrstr, INET_ADDRSTRLEN);
	g_pid = getpid();
	ft_strcpy(g_domain, addrstr);
	if (result->ai_canonname)
		ft_strcpy(g_domain, result->ai_canonname);
	printf("PING %s (%s): %i data bytes\n", FT_TRY(result->ai_canonname, addrstr), addrstr, 64);
	if (fork() == 0)
	{
		signal(SIGINT, sigint_handler);
		rs_clear();
		listener();
		exit(0);
	}
	else
		ping(addr);
	wait(0);
	return (0);
}
