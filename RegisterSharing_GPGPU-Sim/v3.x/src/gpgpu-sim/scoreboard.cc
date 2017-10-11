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

#include "scoreboard.h"
#include "shader.h"
#include "../cuda-sim/ptx_sim.h"
#include "shader_trace.h"


//Constructor
Scoreboard::Scoreboard( unsigned sid, unsigned n_warps )
: longopregs()
{
	m_sid = sid;
	//Initialize size of table
	reg_table.resize(n_warps);
	longopregs.resize(n_warps);
        //lock.resize(n_warps);       
        for(int i=0;i<n_warps; i++)
            lock_warp[i] = -1;
        for(int i=0;i<MAX_CTA_PER_SHADER;i++) {
            lock_cta[i] = -1;
            owner_lock[i] = -1;
        }
        //I have to initialize the lock_cta variables 1
}

// Print scoreboard contents
void Scoreboard::printContents() const
{
	printf("scoreboard contents (sid=%d): \n", m_sid);
	for(unsigned i=0; i<reg_table.size(); i++) {
		if(reg_table[i].size() == 0 ) continue;
		printf("  wid = %2d: ", i);
		std::set<unsigned>::const_iterator it;
		for( it=reg_table[i].begin() ; it != reg_table[i].end(); it++ )
			printf("%u ", *it);
		printf("\n");
	}
}

void Scoreboard::reserveRegister(unsigned wid, unsigned regnum) 
{
	if( !(reg_table[wid].find(regnum) == reg_table[wid].end()) ){
		printf("Error: trying to reserve an already reserved register (sid=%d, wid=%d, regnum=%d).", m_sid, wid, regnum);
        abort();
	}
    SHADER_DPRINTF( SCOREBOARD,
                    "Reserved Register - warp:%d, reg: %d\n", wid, regnum );
	reg_table[wid].insert(regnum);
}

// Unmark register as write-pending
void Scoreboard::releaseRegister(unsigned wid, unsigned regnum) 
{
	if( !(reg_table[wid].find(regnum) != reg_table[wid].end()) ) 
        return;
    SHADER_DPRINTF( SCOREBOARD,
                    "Release register - warp:%d, reg: %d\n", wid, regnum );
	reg_table[wid].erase(regnum);
}

const bool Scoreboard::islongop (unsigned warp_id,unsigned regnum) {
	return longopregs[warp_id].find(regnum) != longopregs[warp_id].end();
}

void Scoreboard::reserveRegisters(const class warp_inst_t* inst) 
{
    for( unsigned r=0; r < 4; r++) {
        if(inst->out[r] > 0) {
            reserveRegister(inst->warp_id(), inst->out[r]);
            SHADER_DPRINTF( SCOREBOARD,
                            "Reserved register - warp:%d, reg: %d\n",
                            inst->warp_id(),
                            inst->out[r] );
        }
    }

    //Keep track of long operations
    if (inst->is_load() &&
    		(	inst->space.get_type() == global_space ||
    			inst->space.get_type() == local_space ||
                inst->space.get_type() == param_space_kernel ||
                inst->space.get_type() == param_space_local ||
                inst->space.get_type() == param_space_unclassified ||
    			inst->space.get_type() == tex_space)){
    	for ( unsigned r=0; r<4; r++) {
    		if(inst->out[r] > 0) {
                SHADER_DPRINTF( SCOREBOARD,
                                "New longopreg marked - warp:%d, reg: %d\n",
                                inst->warp_id(),
                                inst->out[r] );
                longopregs[inst->warp_id()].insert(inst->out[r]);
            }
    	}
    }
}

// Release registers for an instruction
void Scoreboard::releaseRegisters(const class warp_inst_t *inst) 
{
    for( unsigned r=0; r < 4; r++) {
        if(inst->out[r] > 0) {
            SHADER_DPRINTF( SCOREBOARD,
                            "Register Released - warp:%d, reg: %d\n",
                            inst->warp_id(),
                            inst->out[r] );
            releaseRegister(inst->warp_id(), inst->out[r]);
            longopregs[inst->warp_id()].erase(inst->out[r]);
           
        }
    }
}

/** 
 * Checks to see if registers used by an instruction are reserved in the scoreboard
 *  
 * @return 
 * true if WAW or RAW hazard (no WAR since in-order issue)
 **/ 
bool Scoreboard::checkCollision( unsigned wid, const class inst_t *inst ) const
{
	// Get list of all input and output registers
	std::set<int> inst_regs;

	if(inst->out[0] > 0) inst_regs.insert(inst->out[0]);        
	if(inst->out[1] > 0) inst_regs.insert(inst->out[1]);
	if(inst->out[2] > 0) inst_regs.insert(inst->out[2]);
	if(inst->out[3] > 0) inst_regs.insert(inst->out[3]);
	if(inst->in[0] > 0) inst_regs.insert(inst->in[0]);
	if(inst->in[1] > 0) inst_regs.insert(inst->in[1]);
	if(inst->in[2] > 0) inst_regs.insert(inst->in[2]);
	if(inst->in[3] > 0) inst_regs.insert(inst->in[3]);
	if(inst->pred > 0) inst_regs.insert(inst->pred);
	if(inst->ar1 > 0) inst_regs.insert(inst->ar1);
	if(inst->ar2 > 0) inst_regs.insert(inst->ar2);
        
	// Check for collision, get the intersection of reserved registers and instruction registers
	std::set<int>::const_iterator it2;
	for ( it2=inst_regs.begin() ; it2 != inst_regs.end(); it2++ )
		if(reg_table[wid].find(*it2) != reg_table[wid].end()) {
			return true;
		}
	return false;
}

bool Scoreboard::inst_reg_in_unshared_region( const class inst_t *inst, float thresh_hold ) const
{
	// Getting list of all input and output registers
	std::set<int> inst_regs;

	if(inst->out[0] > 0) inst_regs.insert(inst->out[0]);
	if(inst->out[1] > 0) inst_regs.insert(inst->out[1]);
	if(inst->out[2] > 0) inst_regs.insert(inst->out[2]);
	if(inst->out[3] > 0) inst_regs.insert(inst->out[3]);
	if(inst->in[0] > 0) inst_regs.insert(inst->in[0]);
	if(inst->in[1] > 0) inst_regs.insert(inst->in[1]);
	if(inst->in[2] > 0) inst_regs.insert(inst->in[2]);
	if(inst->in[3] > 0) inst_regs.insert(inst->in[3]);
	if(inst->pred > 0) inst_regs.insert(inst->pred);
	if(inst->ar1 > 0) inst_regs.insert(inst->ar1);
	if(inst->ar2 > 0) inst_regs.insert(inst->ar2);        
	
        
	std::set<int>::const_iterator it2;        
	for ( it2=inst_regs.begin() ; it2 != inst_regs.end(); it2++ ) {            
            //if(*it2 > (int)(reg_per_thread * (1- thresh_hold))) {        
            if(*it2 > (int)(reg_per_thread * thresh_hold)) { 
                return false;
            }
        }       
        return true;
}

bool Scoreboard::is_owner(unsigned shared_wid, unsigned wid, int shared_cta, int cta_id) 
{
    /* I need to handle the initial cases properly */ 
    
    if(((shared_cta == -1) || (lock_cta[shared_cta] == cta_id) || owner_lock[shared_cta] == cta_id)  && (wid != -1)) {
         return true;
     }
     else {
         return false;
     }
}
/* Approach-3 and Approach-4 */
bool Scoreboard::check_access_algo_3( unsigned shared_wid, unsigned wid, const class inst_t *inst, int shared_cta, int cta_id, float thresh_hold )
{       
        int old_val = -1;
        int new_val = cta_id;
        bool islocked = false; 
        bool is_owner_locked = false;
       
        
        if((shared_cta == -1) || (inst_reg_in_unshared_region(inst,thresh_hold))) { 
            if(shared_cta != -1) {
                is_owner_locked = owner_lock[shared_cta].compare_exchange_strong( old_val, new_val);                 
            }
            return true;
        }        
        if((lock_cta[shared_cta]==cta_id) || (lock_warp[shared_wid] == wid  )){               
            lock_warp[shared_wid] = wid;
            return true;
        }
        else {                 
            islocked = lock_cta[shared_cta].compare_exchange_strong( old_val, new_val);  
            if(islocked) {  
                lock_warp[shared_wid] = wid;
                return true;
            }
            else { 
                return false;
            } 
        }  
         
}
/* Approach - 2 */
bool Scoreboard::check_access_algo_2( unsigned shared_wid, unsigned wid, const class inst_t *inst, int shared_cta, int cta_id )
{   
    int old_val = -1;
    int new_val = cta_id;
    bool islocked = false;

    if((shared_cta == -1)||(lock_cta[shared_cta]==cta_id) || (lock_warp[shared_wid] == wid  ))
       return true;
    else {                 
        islocked = lock_cta[shared_cta].compare_exchange_strong( old_val, new_val);  
        if(islocked) { return true;}
        else {return false;} 
    }       
}

void Scoreboard::unlock_warp_algo_4(int shared_wid, int wid, int no_warps, int cta_unshare_mode) {
    if(shared_wid == -1 ) return;
    if(wid == shared_wid ) {
        lock_warp[shared_wid] = wid - cta_unshare_mode*no_warps;    
    }
    else lock_warp[shared_wid] = shared_wid;  
    //printf("Unlocking warp, shared_wid  : (%d,%d) \n",wid,shared_wid);    
}





void Scoreboard::unlock_warp(int shared_wid, int wid, int no_warps) {
    if(shared_wid == -1 ) return;
    if(wid == shared_wid ) {
        lock_warp[shared_wid] = wid - no_warps;            
    }
    else lock_warp[shared_wid] = shared_wid;   
   
}
void Scoreboard::unlock_cta_algo_4(int shared_cta, int cta, int cta_unshare_mode) {   
    int x;
    if(shared_cta == -1 ) return;
    if(cta == shared_cta ) {        
        lock_cta[shared_cta] = cta - cta_unshare_mode;
        owner_lock[shared_cta] = cta - cta_unshare_mode;
    }
    else  {
        lock_cta[shared_cta] = shared_cta;
        owner_lock[shared_cta] = shared_cta;
    }
    
}




void Scoreboard::unlock_cta(int shared_cta, int cta) {   
    if(shared_cta == -1 ) return;
    if(cta == shared_cta ) {
        lock_cta[shared_cta] = cta - 1;   
        owner_lock[shared_cta] = cta - 1;   
        
    }
    else {        
        lock_cta[shared_cta] = shared_cta;
        owner_lock[shared_cta] = shared_cta;
        
    }
}

bool Scoreboard::pendingWrites(unsigned wid) const
{
	return !reg_table[wid].empty();
}
