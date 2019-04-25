/* Copyright 2019-present Cisco Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Andy Fingerhut (jafinger@cisco.com)
 *
 */

#ifndef SIMPLE_SWITCH_REGISTER_ACCESS_H_
#define SIMPLE_SWITCH_REGISTER_ACCESS_H_


class RegisterAccess {
 public:
    static constexpr int PACKET_LENGTH_REG_IDX = 0;

    static constexpr int CLONE_MIRROR_SESSION_ID_REG_IDX = 1;
    static constexpr uint64_t CLONE_MIRROR_SESSION_ID_MASK = 0x000000000000ffff;
    static constexpr uint64_t CLONE_MIRROR_SESSION_ID_SHIFT = 0;
    static constexpr int CLONE_FIELD_LIST_REG_IDX = 1;
    static constexpr uint64_t CLONE_FIELD_LIST_MASK = 0x00000000ffff0000;
    static constexpr uint64_t CLONE_FIELD_LIST_SHIFT = 16;
    static constexpr int LF_FIELD_LIST_REG_IDX = 1;
    static constexpr uint64_t LF_FIELD_LIST_MASK = 0x0000ffff00000000;
    static constexpr uint64_t LF_FIELD_LIST_SHIFT = 32;
    static constexpr int RESUBMIT_FLAG_REG_IDX = 1;
    static constexpr uint64_t RESUBMIT_FLAG_MASK = 0xffff000000000000;
    static constexpr uint64_t RESUBMIT_FLAG_SHIFT = 48;

    static constexpr int RECIRCULATE_FLAG_REG_IDX = 2;
    static constexpr uint64_t RECIRCULATE_FLAG_MASK = 0x000000000000ffff;
    static constexpr uint64_t RECIRCULATE_FLAG_SHIFT = 0;

    static constexpr uint16_t MAX_MIRROR_SESSION_ID = (1u << 15) - 1;
    static constexpr uint16_t MIRROR_SESSION_ID_VALID_MASK = (1u << 15);
    static constexpr uint16_t MIRROR_SESSION_ID_MASK = 0x7FFFu;

    static void clear_all(Packet *pkt) {
        // except do not clear packet length
        pkt->set_register(1, 0);
        pkt->set_register(2, 0);
    }
    static uint16_t get_clone_mirror_session_id(Packet *pkt) {
        uint64_t rv = pkt->get_register(CLONE_MIRROR_SESSION_ID_REG_IDX);
        return static_cast<uint16_t>((rv & CLONE_MIRROR_SESSION_ID_MASK) >>
                                     CLONE_MIRROR_SESSION_ID_SHIFT);
    }
    static void set_clone_mirror_session_id(Packet *pkt,
                                            uint16_t mirror_session_id) {
        uint64_t rv = pkt->get_register(CLONE_MIRROR_SESSION_ID_REG_IDX);
        rv = ((rv & ~CLONE_MIRROR_SESSION_ID_MASK) |
              ((static_cast<uint64_t>(mirror_session_id)) <<
               CLONE_MIRROR_SESSION_ID_SHIFT));
        pkt->set_register(CLONE_MIRROR_SESSION_ID_REG_IDX, rv);
    }
    static uint16_t get_clone_field_list(Packet *pkt) {
        uint64_t rv = pkt->get_register(CLONE_FIELD_LIST_REG_IDX);
        return static_cast<uint16_t>((rv & CLONE_FIELD_LIST_MASK) >>
                                     CLONE_FIELD_LIST_SHIFT);
    }
    static void set_clone_field_list(Packet *pkt, uint16_t field_list_id) {
        uint64_t rv = pkt->get_register(CLONE_FIELD_LIST_REG_IDX);
        rv = ((rv & ~CLONE_FIELD_LIST_MASK) |
              ((static_cast<uint64_t>(field_list_id)) <<
               CLONE_FIELD_LIST_SHIFT));
        pkt->set_register(CLONE_FIELD_LIST_REG_IDX, rv);
    }
    static uint16_t get_lf_field_list(Packet *pkt) {
        uint64_t rv = pkt->get_register(LF_FIELD_LIST_REG_IDX);
        return static_cast<uint16_t>((rv & LF_FIELD_LIST_MASK) >>
                                     LF_FIELD_LIST_SHIFT);
    }
    static void set_lf_field_list(Packet *pkt, uint16_t field_list_id) {
        uint64_t rv = pkt->get_register(LF_FIELD_LIST_REG_IDX);
        rv = ((rv & ~LF_FIELD_LIST_MASK) |
              ((static_cast<uint64_t>(field_list_id)) << LF_FIELD_LIST_SHIFT));
        pkt->set_register(LF_FIELD_LIST_REG_IDX, rv);
    }
    static uint16_t get_resubmit_flag(Packet *pkt) {
        uint64_t rv = pkt->get_register(RESUBMIT_FLAG_REG_IDX);
        return static_cast<uint16_t>((rv & RESUBMIT_FLAG_MASK) >>
                                     RESUBMIT_FLAG_SHIFT);
    }
    static void set_resubmit_flag(Packet *pkt, uint16_t field_list_id) {
        uint64_t rv = pkt->get_register(RESUBMIT_FLAG_REG_IDX);
        rv = ((rv & ~RESUBMIT_FLAG_MASK) |
              ((static_cast<uint64_t>(field_list_id)) << RESUBMIT_FLAG_SHIFT));
        pkt->set_register(RESUBMIT_FLAG_REG_IDX, rv);
    }
    static uint16_t get_recirculate_flag(Packet *pkt) {
        uint64_t rv = pkt->get_register(RECIRCULATE_FLAG_REG_IDX);
        return static_cast<uint16_t>((rv & RECIRCULATE_FLAG_MASK) >>
                                     RECIRCULATE_FLAG_SHIFT);
    }
    static void set_recirculate_flag(Packet *pkt, uint16_t field_list_id) {
        uint64_t rv = pkt->get_register(RECIRCULATE_FLAG_REG_IDX);
        rv = ((rv & ~RECIRCULATE_FLAG_MASK) |
              ((static_cast<uint64_t>(field_list_id)) <<
               RECIRCULATE_FLAG_SHIFT));
        pkt->set_register(RECIRCULATE_FLAG_REG_IDX, rv);
    }
};

#endif  // SIMPLE_SWITCH_REGISTER_ACCESS_H_
