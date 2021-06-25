/**
 * Contains various types for oldstyle
 */
#ifndef OLDSTYLE_SERVER_OFFER_HPP
#define OLDSTYLE_SERVER_OFFER_HPP

#include <bits/stdint-uintn.h>
#include <endian.h>
#include <netinet/in.h>
#include <sys/types.h>
const uint64_t NBDMAGIC = 0x4e42444d41474943;
const uint64_t CLISERVE_MAGIC = 0x00420281861253;

// see: https://github.com/NetworkBlockDevice/nbd/doc/proto.md
struct OldstyleServerOffer {
  u_int64_t magic;         // magic number
  u_int64_t cliserv_magic; // magic number
  u_int64_t export_size;   // size of the export (bytes)
  u_int16_t gflags;        // 0 for oldstyle global flags
  u_int16_t eflags;        // export flags
  u_int8_t bytes[124];     // reserved

  void hostify() {
    magic = be64toh(magic);
    cliserv_magic = be64toh(cliserv_magic);
    export_size = be64toh(export_size);
    gflags = ntohs(gflags);
    eflags = ntohs(eflags);
  }
} __attribute__((__packed__));

bool is_oldstyle_offer(const OldstyleServerOffer &offer);
void print_server_offer(const OldstyleServerOffer &server_offer);

#endif // OLDSTYLE_SERVER_OFFER_HPP
