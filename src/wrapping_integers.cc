#include "wrapping_integers.hh"

using namespace std;

#define MOD ( static_cast<uint64_t>( 1 ) << 32 )

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return zero_point + n;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  int32_t offset = this->raw_value_ - zero_point.raw_value_;
  uint64_t absoluteSeq = offset >= 0 ? static_cast<uint64_t>( offset )
                                     : ( ( static_cast<int64_t>( 1 ) << 32 ) + static_cast<int64_t>( offset ) );

  if ( absoluteSeq >= checkpoint )
    return absoluteSeq;

  uint64_t difference = checkpoint - absoluteSeq;
  uint64_t quotient = difference >> 32;
  uint64_t remainder = ( difference << 32 ) >> 32;
  absoluteSeq += quotient * MOD;
  if ( remainder > ( MOD >> 1 ) )
    absoluteSeq += MOD;

  return absoluteSeq;
}
