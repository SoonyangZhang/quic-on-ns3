#pragma once
namespace quic{
/*
 * Sender's congestion state indicating normal or abnormal situations
 * in the last round of packets sent. The state is driven by the ACK
 * information and timer events.
 */
enum tcp_ca_state {
	/*
	 * Nothing bad has been observed recently.
	 * No apparent reordering, packet loss, or ECN marks.
	 */
	TCP_CA_Open = 0,
#define TCPF_CA_Open	(1<<TCP_CA_Open)
	/*
	 * The sender enters disordered state when it has received DUPACKs or
	 * SACKs in the last round of packets sent. This could be due to packet
	 * loss or reordering but needs further information to confirm packets
	 * have been lost.
	 */
	TCP_CA_Disorder = 1,
#define TCPF_CA_Disorder (1<<TCP_CA_Disorder)
	/*
	 * The sender enters Congestion Window Reduction (CWR) state when it
	 * has received ACKs with ECN-ECE marks, or has experienced congestion
	 * or packet discard on the sender host (e.g. qdisc).
	 */
	TCP_CA_CWR = 2,
#define TCPF_CA_CWR	(1<<TCP_CA_CWR)
	/*
	 * The sender is in fast recovery and retransmitting lost packets,
	 * typically triggered by ACK events.
	 */
	TCP_CA_Recovery = 3,
#define TCPF_CA_Recovery (1<<TCP_CA_Recovery)
	/*
	 * The sender is in loss recovery triggered by retransmission timeout.
	 */
	TCP_CA_Loss = 4
#define TCPF_CA_Loss	(1<<TCP_CA_Loss)
};
}