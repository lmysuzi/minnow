#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  _routing_table.push_back( RouteInfo { route_prefix, prefix_length, next_hop, interface_num } );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( auto& interface : _interfaces ) {
    auto& queue = interface->datagrams_received();
    while ( not queue.empty() ) {
      route_one_datagram( queue.front() );
      queue.pop();
    }
  }
}

std::optional<Router::RouteInfo> Router::match( InternetDatagram& dgram )
{
  optional<RouteInfo> ans = nullopt;
  uint32_t dst = dgram.header.dst;
  uint8_t max_prefix_length = 0;

  for ( auto& route : _routing_table ) {
    if ( ( route.prefix_length == 0 )
         || ( ( route.route_prefix >> ( 32 - route.prefix_length ) )
              == ( dst >> ( 32 - route.prefix_length ) ) ) ) {
      if ( route.route_prefix >= max_prefix_length ) {
        ans = route;
        max_prefix_length = route.route_prefix;
      }
    }
  }

  return ans;
}

void Router::route_one_datagram( InternetDatagram& dgram )
{
  auto optional_route = match( dgram );
  if ( !optional_route.has_value() )
    return;
  if ( dgram.header.ttl <= 1 )
    return;

  dgram.header.ttl--;
  dgram.header.len = static_cast<uint64_t>( dgram.header.hlen ) * 4 + dgram.payload.back().size();
  dgram.header.compute_checksum();

  size_t interface_num = optional_route.value().interface_num;
  if ( optional_route.value().next_hop.has_value() ) {
    _interfaces[interface_num]->send_datagram( dgram, optional_route.value().next_hop.value() );
  } else {
    _interfaces[interface_num]->send_datagram( dgram, Address::from_ipv4_numeric( dgram.header.dst ) );
  }
}