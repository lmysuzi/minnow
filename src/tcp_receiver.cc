#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if ( message.RST ) {
    reader().set_error();
    return;
  }
  Wrap32 seqno( message.seqno );
  if ( message.SYN ) {
    isn_ = seqno;
    is_isn_set_ = true;
    reassembler_.insert( 0, message.payload, message.FIN );
    return;
  } else if ( !is_isn_set_ ) {
    return;
  }
  uint64_t stream_index = seqno.unwrap( isn_, writer().bytes_pushed() );
  if ( stream_index == 0 )
    return;
  stream_index--;
  //超出窗口
  if ( stream_index >= writer().bytes_pushed() + writer().available_capacity() ) {
    return;
  }
  reassembler_.insert( stream_index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage message = TCPReceiverMessage();
  message.window_size = min( writer().available_capacity(), (uint64_t)UINT16_MAX );
  message.RST = writer().has_error();

  if ( !is_isn_set_ ) {
    message.ackno = std::nullopt;
    return message;
  }
  Wrap32 ackno = Wrap32::wrap( writer().bytes_pushed() + 1, isn_ );
  if ( writer().is_closed() )
    ackno = ackno + uint32_t( 1 );
  message.ackno = ackno;

  return message;
}
