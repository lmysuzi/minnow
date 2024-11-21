#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  const uint32_t next_hop_ip = next_hop.ipv4_numeric();

  if ( mapping_existing( next_hop_ip ) ) {
    send_ipv4( dgram, address_mapping_.at( next_hop_ip ).addr );
  } else {
    if ( arp_time_mapping_.find( next_hop_ip ) == arp_time_mapping_.end()
         || arp_time_mapping_[next_hop_ip] + 5000 < time_passed_ ) {
      arp_time_mapping_[next_hop_ip] = time_passed_;
      send_arp( next_hop_ip, ETHERNET_BROADCAST );
      datagram_mapping_.insert( pair<uint32_t, InternetDatagram>( next_hop_ip, dgram ) );
    }
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 && frame.header.dst == ethernet_address_ ) {
    InternetDatagram datagram;

    if ( parse( datagram, frame.payload ) ) {
      datagrams_received_.push( datagram );
    }
  } else if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp;
    if ( parse( arp, frame.payload ) ) {
      address_mapping_[arp.sender_ip_address] = EthernetInfo { arp.sender_ethernet_address, time_passed_ };
      if ( arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == ip_address_.ipv4_numeric() ) {
        send_arp( arp.sender_ip_address, arp.sender_ethernet_address );
      }
      // send ipv4
      for ( auto begin = datagram_mapping_.lower_bound( arp.sender_ip_address ),
                 end = datagram_mapping_.upper_bound( arp.sender_ip_address );
            begin != end; ) {
        send_ipv4( begin->second, arp.sender_ethernet_address );
        datagram_mapping_.erase( begin++ );
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  time_passed_ += ms_since_last_tick;
}

bool NetworkInterface::mapping_existing( const uint32_t next_hop_ip )
{
  bool flag = false;
  if ( address_mapping_.find( next_hop_ip ) != address_mapping_.end() ) {
    EthernetInfo info = address_mapping_.at( next_hop_ip );
    if ( info.coming_time + TIME_EXIST < time_passed_ ) {
      address_mapping_.erase( next_hop_ip );
    } else {
      flag = true;
    }
  }
  return flag;
}

void NetworkInterface::send_ipv4( const InternetDatagram& dgram, const EthernetAddress& target_ethernet_address )
{
  EthernetFrame frame;
  frame.header.src = ethernet_address_;
  frame.header.dst = target_ethernet_address;
  frame.header.type = EthernetHeader::TYPE_IPv4;
  frame.payload = serialize( dgram );
  transmit( frame );
}

void NetworkInterface::send_arp( const uint32_t next_hop_ip, const EthernetAddress& target_ethernet_address )
{
  ARPMessage arp;
  EthernetFrame frame;

  arp.sender_ethernet_address = ethernet_address_;
  arp.sender_ip_address = ip_address_.ipv4_numeric();
  arp.target_ip_address = next_hop_ip;
  frame.header.src = ethernet_address_;
  frame.header.type = EthernetHeader::TYPE_ARP;
  if ( target_ethernet_address == ETHERNET_BROADCAST ) {
    arp.target_ethernet_address = { 0, 0, 0, 0, 0, 0 };
    arp.opcode = ARPMessage::OPCODE_REQUEST;
    frame.header.dst = ETHERNET_BROADCAST;
  } else {
    arp.target_ethernet_address = target_ethernet_address;
    arp.opcode = ARPMessage::OPCODE_REPLY;
    frame.header.dst = target_ethernet_address;
  }
  frame.payload = serialize( arp );
  transmit( frame );
}