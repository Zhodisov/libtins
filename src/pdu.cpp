/*
 * libtins is a net packet wrapper library for crafting and
 * interpreting sniffed packets.
 *
 * Copyright (C) 2011 Nasel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <cassert>
#include <iostream> //borrame
#include "utils.h"
#include "pdu.h"
#include "rawpdu.h"


Tins::PDU::PDU(uint32_t flag, PDU *next_pdu) : _flag(flag), _inner_pdu(next_pdu) {

}

Tins::PDU::~PDU() {
    delete _inner_pdu;
}

uint32_t Tins::PDU::size() const {
    uint32_t sz = header_size() + trailer_size();
    const PDU *ptr(_inner_pdu);
    while(ptr) {
        sz += ptr->header_size() + trailer_size();
        ptr = ptr->inner_pdu();
    }
    return sz;
}

void Tins::PDU::flag(uint32_t new_flag) {
    _flag = new_flag;
}

void Tins::PDU::inner_pdu(PDU *next_pdu) {
    delete _inner_pdu;
    _inner_pdu = next_pdu;
}

uint8_t *Tins::PDU::serialize(uint32_t &sz) {
    sz = size();
    uint8_t *buffer = new uint8_t[sz];
    serialize(buffer, sz, 0);
    return buffer;
}

void Tins::PDU::serialize(uint8_t *buffer, uint32_t total_sz, const PDU *parent) {
    uint32_t sz = header_size() + trailer_size();
    /* Must not happen... */
    assert(total_sz >= sz);
    if(_inner_pdu)
        _inner_pdu->serialize(buffer + header_size(), total_sz - sz, this);
    write_serialization(buffer, total_sz, parent);
}

Tins::PDU *Tins::PDU::clone_inner_pdu(uint8_t *ptr, uint32_t total_sz) {
    PDU *child = 0;
    if(inner_pdu()) {
        child = inner_pdu()->clone_packet(ptr, total_sz);
        if(!child)
            return 0;
    }
    else
        child = new RawPDU(ptr, total_sz, true);
    return child;
}

/* Static methods */
uint32_t Tins::PDU::do_checksum(uint8_t *start, uint8_t *end) {
    uint32_t checksum(0);
    uint16_t *ptr = (uint16_t*)start, *last = (uint16_t*)end, padding(0);
    if(((end - start) & 1) == 1) {
        last = (uint16_t*)end - 1;
        padding = *(end - 1) << 8;
    }
    while(ptr < last)
        checksum += Utils::net_to_host_s(*(ptr++));
    return checksum + padding;
}

uint32_t Tins::PDU::pseudoheader_checksum(uint32_t source_ip, uint32_t dest_ip, uint32_t len, uint32_t flag) {
    uint32_t checksum(0);
    source_ip = Utils::net_to_host_l(source_ip);
    dest_ip = Utils::net_to_host_l(dest_ip);
    uint16_t *ptr = (uint16_t*)&source_ip;

    checksum += (uint32_t)(*ptr) + (uint32_t)(*(ptr+1));
    ptr = (uint16_t*)&dest_ip;
    checksum += (uint32_t)(*ptr) + (uint32_t)(*(ptr+1));
    checksum += flag + len;
    return checksum;
}

