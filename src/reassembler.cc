#include "reassembler.hh"
#include <iostream>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  uint64_t available_capacity = writer().available_capacity();
  for ( auto i = unassembled_buffer_.size(); i < available_capacity; i++ ) {
    unassembled_buffer_.push_back( '\0' );
    existing_buffer_.push_back( false );
  }

  // 确定范围
  uint64_t buffer_begin_index = 0;
  uint64_t data_begin_index = 0;
  if ( current_index_ <= first_index ) {
    buffer_begin_index = first_index - current_index_;
  } else {
    data_begin_index = current_index_ - first_index;
  }

  uint64_t i = data_begin_index;
  uint64_t j = buffer_begin_index;
  for ( ; i < data.length() && j < available_capacity; i++, j++ ) {
    if ( existing_buffer_.at( j ) == false ) {
      unassembled_buffer_.at( j ) = data.at( i );
      existing_buffer_.at( j ) = true;
      unassembled_bytes_++;
    }
  }

  if ( i == data.length() && is_last_substring ) {
    eof_ = true;
    eof_index_ = j;
  }

  // 写入stream
  std::string str = "";
  while ( !unassembled_buffer_.empty() && existing_buffer_.front() ) {
    str += unassembled_buffer_.front();
    unassembled_buffer_.pop_front();
    existing_buffer_.pop_front();
    unassembled_bytes_--;
    current_index_++;
  }
  output_.writer().push( str );
  if ( eof_ ) {
    eof_index_ -= str.length();
    if ( eof_index_ == 0 ) {
      output_.writer().close();
    }
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return unassembled_bytes_;
}