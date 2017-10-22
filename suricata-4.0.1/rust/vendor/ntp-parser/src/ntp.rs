use nom::{be_i8,be_u8,be_u16,be_u32,be_u64};

use enum_primitive::FromPrimitive;

enum_from_primitive! {
#[derive(Debug,PartialEq)]
#[repr(u8)]
pub enum NtpMode {
    Reserved = 0,
    SymmetricActive = 1,
    SymmetricPassive = 2,
    Client = 3,
    Server = 4,
    Broadcast = 5,
    NtpControlMessage = 6,
    Private = 7,
}
}

#[derive(Debug,PartialEq)]
pub struct NtpPacket<'a> {
    pub li: u8,
    pub version: u8,
    pub mode: u8,
    pub stratum: u8,
    pub poll: i8,
    pub precision: i8,
    pub root_delay: u32,
    pub root_dispersion: u32,
    pub ref_id:u32,
    pub ts_ref:u64,
    pub ts_orig:u64,
    pub ts_recv:u64,
    pub ts_xmit:u64,

    pub ext_and_auth:Option<(Vec<NtpExtension<'a>>, (u32, &'a[u8]))>,
    pub auth: Option<(u32,&'a[u8])>,
}

impl<'a> NtpPacket<'a> {
    #[inline]
    pub fn get_mode(&self) -> Option<NtpMode> {
        NtpMode::from_u8(self.mode)
    }

    pub fn get_precision(&self) -> f32 {
        2.0_f32.powf(self.precision as f32)
    }
}

#[derive(Debug,PartialEq)]
pub struct NtpExtension<'a> {
    pub field_type: u16,
    pub length: u16,
    pub value: &'a[u8],
    /*padding*/
}

named!(pub parse_ntp_extension<NtpExtension>,
    do_parse!(
           ty: be_u16
        >> len: be_u16 // len includes the padding
        >> data: take!(len)
        >> (
            NtpExtension{
                field_type:ty,
                length:len,
                value:data,
            }
        ))
);

named!(pub parse_ntp_key_mac<(u32,&[u8])>,
   complete!(pair!(be_u32,take!(16)))
);

named!(pub parse_ntp<NtpPacket>,
   do_parse!(
          b0: bits!(
                  tuple!(take_bits!(u8,2),take_bits!(u8,3),take_bits!(u8,3))
              )
       >> st: be_u8
       >> pl: be_i8
       >> pr: be_i8
       >> rde: be_u32
       >> rdi: be_u32
       >> rid: be_u32
       >> tsr: be_u64
       >> tso: be_u64
       >> tsv: be_u64
       >> tsx: be_u64
       // optional fields, See section 7.5 of [RFC5905] and [RFC7822]
       // extensions, key ID and MAC
       >> extn: opt!(complete!(pair!(many0!(complete!(parse_ntp_extension)),parse_ntp_key_mac)))
       >> auth: opt!(parse_ntp_key_mac)
       >> (
           NtpPacket {
               li:b0.0,
               version:b0.1,
               mode:b0.2,
               stratum:st,
               poll:pl,
               precision:pr,
               root_delay:rde,
               root_dispersion:rdi,
               ref_id:rid,
               ts_ref:tsr,
               ts_orig:tso,
               ts_recv:tsv,
               ts_xmit:tsx,
               ext_and_auth:extn,
               auth:auth,
           }
   ))
);

#[cfg(test)]
mod tests {
    use ntp::*;
    use nom::IResult;

static NTP_REQ1: &'static [u8] = &[
    0xd9, 0x00, 0x0a, 0xfa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x90,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xc5, 0x02, 0x04, 0xec, 0xec, 0x42, 0xee, 0x92
];

#[test]
fn test_ntp_packet1() {
    let empty = &b""[..];
    let bytes = NTP_REQ1;
    let expected = IResult::Done(empty,NtpPacket{
        li:3,
        version:3,
        mode:1,
        stratum:0,
        poll:10,
        precision:-6,
        root_delay:0,
        root_dispersion:0x010290,
        ref_id:0,
        ts_ref:0,
        ts_orig:0,
        ts_recv:0,
        ts_xmit:14195914391047827090u64,
        ext_and_auth:None,
        auth:None,
    });
    let res = parse_ntp(&bytes);
    assert_eq!(res, expected);
}

static NTP_REQ2: &'static [u8] = &[
    0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xcc, 0x25, 0xcc, 0x13, 0x2b, 0x02, 0x10, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x52, 0x80, 0x0c, 0x2b, 0x59, 0x00, 0x64, 0x66,
    0x84, 0xf4, 0x4c, 0xa4, 0xee, 0xce, 0x12, 0xb8
];

#[test]
fn test_ntp_packet2() {
    let empty = &b""[..];
    let bytes = NTP_REQ2;
    let expected = IResult::Done(empty,NtpPacket{
        li:0,
        version:4,
        mode:3,
        stratum:0,
        poll:0,
        precision:0,
        root_delay:12,
        root_dispersion:0,
        ref_id:0,
        ts_ref:0,
        ts_orig:0,
        ts_recv:0,
        ts_xmit:14710388140573593600,
        ext_and_auth:None,
        auth:Some((1,&bytes[52..])),
    });
    let res = parse_ntp(&bytes);
    assert_eq!(res, expected);
}

}
