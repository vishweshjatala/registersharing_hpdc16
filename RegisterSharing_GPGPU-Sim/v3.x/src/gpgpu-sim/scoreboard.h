// Copyright (c) 2009-2011, Tor M. Aamodt, Inderpreet Singh
// The University of British Columbia
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this
// list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
// Neither the name of The University of British Columbia nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <set>
#include "assert.h"
#include <cstdatomic> //Vishwesh

#include "shader.h"


#ifndef SCOREBOARD_H_
#define SCOREBOARD_H_

#include "../abstract_hardware_model.h"

/* Modified by Vishwesh
 * These two values should be same as MAX_CTA_PER_SHADER, WARP_PER_CTA_MAX as defined in shader.h*/
#define DUP_MAX_CTA_PER_SHADER 32
#define DUP_WARP_PER_CTA_MAX 128

class Scoreboard {
public:
    Scoreboard( unsigned sid, unsigned n_warps );

    void reserveRegisters(const warp_inst_t *inst);
    void releaseRegisters(const warp_inst_t *inst);
    void releaseRegister(unsigned wid, unsigned regnum);

    bool checkCollision(unsigned wid, const inst_t *inst) const;
    bool pendingWrites(unsigned wid) const;
    void printContents() const;
    const bool islongop(unsigned warp_id, unsigned regnum);
    bool check_access_algo_3( unsigned shared_wid, unsigned wid, const class inst_t *inst, int shared_cta, int cta_id, float thresh_hold );
    bool check_access_algo_2( unsigned shared_wid, unsigned wid, const class inst_t *inst, int shared_cta, int cta_id );
    void unlock_warp(int shared_wid, int wid, int no_warps);    
    void unlock_cta(int shared_cta, int cta);
    //void unlock_warp_algo_4(int shared_wid, int wid, int no_warps, int no_shared_pairs);  
    void unlock_warp_algo_4(int shared_wid, int wid, int no_warps, int cta_unshare_mode);  
    //void unlock_cta_algo_4(int shared_cta, int cta, int no_shared_pairs);
    void unlock_cta_algo_4(int shared_cta, int cta, int cta_unshare_mode);
    bool is_owner(unsigned shared_wid, unsigned wid, int shared_cta, int cta_id); 
    void set_reg_per_thread(int reg) { reg_per_thread = reg; }
    bool inst_reg_in_unshared_region( const class inst_t *inst, float thresh_hold ) const ;

private:
    void reserveRegister(unsigned wid, unsigned regnum);
    int get_sid() const { return m_sid; }
   

    unsigned m_sid;

    // keeps track of pending writes to registers
    // indexed by warp id, reg_id => pending write count
    std::vector< std::set<unsigned> > reg_table;
    bool occupied;
    //Register that depend on a long operation (global, local or tex memory)
    std::vector< std::set<unsigned> > longopregs;
    std::atomic<int> lock_cta[DUP_MAX_CTA_PER_SHADER]; 
    std::atomic<int> owner_lock[DUP_MAX_CTA_PER_SHADER]; 
    std::atomic<int> lock_warp[DUP_WARP_PER_CTA_MAX];
    //CTA_SIZE/warp_size
    int MAX_UNSHARED_REG;
    int reg_per_thread;
};


#endif /* SCOREBOARD_H_ */
