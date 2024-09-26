#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), remaining_capacity_( capacity ) {}

bool Writer::is_closed() const
{
  // Your code here.
  return closed_;
}

void Writer::push( string data )
{
  // Your code here.
  if ( closed_ )
    return;

  uint64_t wirtten_count = data.length() <= remaining_capacity_ ? data.length() : remaining_capacity_;
  buffer_ += data.substr( 0, wirtten_count );
  remaining_capacity_ -= wirtten_count;
  bytes_written_ += wirtten_count;
}

void Writer::close()
{
  // Your code here.
  closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return remaining_capacity_;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_written_;
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed_ && ( buffer_.length() == 0 );
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_poped_;
}

string_view Reader::peek() const
{
  // Your code here.
  return std::string_view( buffer_ );
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t len_poped = len <= buffer_.length() ? len : buffer_.length();
  buffer_.erase( buffer_.begin(), buffer_.begin() + len_poped );
  remaining_capacity_ += len_poped;
  bytes_poped_ += len_poped;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer_.length();
}
