#include "clic.h"
#include "processor.h"
#include "arith.h"
#include "dts.h"
#include "sim.h"
#include "mmu.h"


// M-mode CLIC memory map - 12/19/2023 - version 0.9-draft
//   Offset
//   0x0000         4B          RW        reserved for smclicconfig extension
//
//// 0x0004-0x003F              reserved    ###
//
//   0x0040         4B          RW        clicinttrig[0]
//   0x0044         4B          RW        clicinttrig[1]
//   0x0048         4B          RW        clicinttrig[2]
//   ...
//   0x00B4         4B          RW        clicinttrig[29]
//   0x00B8         4B          RW        clicinttrig[30]
//   0x00BC         4B          RW        clicinttrig[31]
//
//// 0x00C0-0x07FF              reserved    ###
//// 0x0800-0x0FFF              custom      ###
//
//   0x1000+4*i     1B/input    R or RW   clicintip[i]
//   0x1001+4*i     1B/input    RW        clicintie[i]
//   0x1002+4*i     1B/input    RW        clicintattr[i]
//   0x1003+4*i     1B/input    RW        clicintctl[i]
//   ...
//   0x4FFC         1B/input    R or RW   clicintip[4095]
//   0x4FFD         1B/input    RW        clicintie[4095]
//   0x4FFE         1B/input    RW        clicintattr[4095]
//   0x4FFF         1B/input    RW        clicintctl[4095]

#define MCLIC_SMCLICCONFIG_EXT_OFFSET  0X0000

#define MCLIC_RESERVED1_BASE_OFFSET    0X0004
#define MCLIC_RESERVED1_TOP_OFFSET     0X003F

#define MCLIC_INTTRIG_ADDR_BASE_OFFSET 0X0040
#define MCLIC_INTTRIG_ADDR_TOP_OFFSET  0X00BC

#define MCLIC_RESEVED2_BASE_OFFSET     0X00C0
#define MCLIC_RESEVED2_TOP_OFFSET      0X07FF

#define MCLIC_CUSTOM_BASE_OFFSET       0X0800
#define MCLIC_CUSTOM_TOP_OFFSET        0X0FFF

#define MCLIC_INTTBL_ADDR_BASE_OFFSET  0X1000
#define     MCLIC_INTIP_BYTE_OFFSET     0
#define     MCLIC_INTIE_BYTE_OFFSET     1
#define     MCLIC_INTATTR_BYTE_OFFSET   2
#define     MCLIC_INTCTL_BYTE_OFFSET    3
#define MCLIC_INTTBL_ADDR_TOP_OFFSET   0X4FFC

// Layout of user-mode CLIC regions - 12/19/2023 - version 0.9-draft
// Offset
// 0x0000       4B          RW        reserved for smclicconfig extension
//
// 0x0004-0x07FF              reserved    ###
//
// 0x0800-0x0FFF              custom      ###
//
// 0x1000+4*i   1B/input    R or RW   clicintip[i]
// 0x1001+4*i   1B/input    RW        clicintie[i]
// 0x1002+4*i   1B/input    RW        clicintattr[i]
// 0x1003+4*i   1B/input    RW        clicintctl[i]
#define UCLIC_SMCLICCONFIG_EXT_OFFSET  0X0000

#define UCLIC_RESERVED_BASE_OFFSET    0X0004
#define UCLIC_RESERVED_TOP_OFFSET     0X07FF

#define UCLIC_CUSTOM_BASE_OFFSET       0X0800
#define UCLIC_CUSTOM_TOP_OFFSET        0X0FFF

#define UCLIC_INTTBL_ADDR_BASE_OFFSET  0X1000
#define     UCLIC_INTIP_BYTE_OFFSET     0
#define     UCLIC_INTIE_BYTE_OFFSET     1
#define     UCLIC_INTATTR_BYTE_OFFSET   2
#define     UCLIC_INTCTL_BYTE_OFFSET    3
#define UCLIC_INTTBL_ADDR_TOP_OFFSET   0X4FFC

clic_t::CLICINTTRIG_UNION_T clic_t::clicinttrig[CLIC_NUM_TRIGGER]   = {0};
uint8_t                     clic_t::clicintip[CLIC_NUM_INTERRUPT]   = {0};
uint8_t                     clic_t::clicintie[CLIC_NUM_INTERRUPT]   = {0};
clic_t::CLICINTATTR_UNION_T clic_t::clicintattr[CLIC_NUM_INTERRUPT] = {0};
uint8_t                     clic_t::clicintctl[CLIC_NUM_INTERRUPT]  = {0};

void clic_t::set_smclic_enabled(bool val) {
  SMCLIC_enabled = val;
}
void clic_t::set_suclic_enabled(bool val) {
  SUCLIC_enabled = val;
}
void clic_t::set_smclicshv_enabled(bool val) {
  SMCLICSHV_enabled = val;
}
bool clic_t::get_smclic_enabled() {
  return SMCLIC_enabled;
}
bool clic_t::get_suclic_enabled() {
  return SUCLIC_enabled;
}
bool clic_t::get_smclicshv_enabled() {
  return SMCLICSHV_enabled;
}

bool clic_t::load(reg_t addr, size_t len, uint8_t *bytes)  {
  if (len > 8)
    return false;

  tick(0);

  if ((addr >= MCLIC_SMCLICCONFIG_EXT_OFFSET) && (addr < MCLIC_RESERVED1_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    for (int indx = 0; indx < len; indx++)
    {
      // FIXME add cliccfg register when extension smclicconfig is enabled
      bytes[indx] = 0;
    }
    return true;
  } else if ((addr >= MCLIC_RESERVED1_BASE_OFFSET) && (addr < MCLIC_INTTRIG_ADDR_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    for (int indx = 0; indx < len; indx++)
    {
      bytes[indx] = 0;
    }
    return true;
  } else if ((addr >= MCLIC_INTTRIG_ADDR_BASE_OFFSET) && (addr < MCLIC_RESEVED2_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    /* load from clic interrupt trigger memory mapped registers */
    int index = addr - MCLIC_INTTRIG_ADDR_BASE_OFFSET;
    for (int i = 0; i < len; i++)
    {
      bytes[i] = clicinttrig[index].all >> i*8;
    }
    return true;
  } else if ((addr >= MCLIC_CUSTOM_BASE_OFFSET ) && (addr < MCLIC_INTTBL_ADDR_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    for (int indx = 0; indx < len; indx++)
    {
      bytes[indx] = 0;
    }
    return true;
  } else if ((addr >= MCLIC_INTTBL_ADDR_BASE_OFFSET ) && (addr < (MCLIC_INTTBL_ADDR_TOP_OFFSET + 4))) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    int index = ((addr & 0XFFFFFFFFFFFFFFFC) - MCLIC_INTTBL_ADDR_BASE_OFFSET) / 4;
    int byte_offset = addr & 0x3;
    for (int i = byte_offset; i < byte_offset + len; i++)
    {
      switch (i)
      {
      case MCLIC_INTIP_BYTE_OFFSET:
        bytes[i] = clicintip[index];
        break;
      case MCLIC_INTIE_BYTE_OFFSET:
        bytes[i] = clicintie[index];
        break;
      case MCLIC_INTATTR_BYTE_OFFSET:
        bytes[i] = clicintattr[index].all;
        break;
      case MCLIC_INTCTL_BYTE_OFFSET:
        bytes[i] = clicintctl[index];
        break;
      default:
        return false;
        break;
      }
    }
    return true;
  } else if ((addr >= UCLIC_SMCLICCONFIG_EXT_OFFSET) && (addr < UCLIC_RESERVED_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    for (int indx = 0; indx < len; indx++)
    {
      // FIXME add cliccfg register when extension suclicconfig is enabled
      bytes[indx] = 0;
    }
    return true;
  } else if ((addr >= UCLIC_RESERVED_BASE_OFFSET) && (addr < UCLIC_CUSTOM_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    for (int indx = 0; indx < len; indx++)
    {
      bytes[indx] = 0;
    }
    return true;
  } else if ((addr >= UCLIC_CUSTOM_BASE_OFFSET ) && (addr < UCLIC_INTTBL_ADDR_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    for (int indx = 0; indx < len; indx++)
    {
      bytes[indx] = 0;
    }
    return true;
  } else if ((addr >= UCLIC_INTTBL_ADDR_BASE_OFFSET ) && (addr < (UCLIC_INTTBL_ADDR_TOP_OFFSET + 4))) {
    if (len == 8) {
      // Implement double-word loads as a pair of word loads
      return load(addr, 4, bytes) && load(addr + 4, 4, bytes + 4);
    }
    int index = ((addr & 0XFFFFFFFFFFFFFFFC) - UCLIC_INTTBL_ADDR_BASE_OFFSET) / 4;
    int byte_offset = addr & 0x3;
    for (int i = byte_offset; i < byte_offset + len; i++)
    {
      switch (i)
      {
      case UCLIC_INTIP_BYTE_OFFSET:
        bytes[i] = clicintip[index];
        break;
      case UCLIC_INTIE_BYTE_OFFSET:
        bytes[i] = clicintie[index];
        break;
      case UCLIC_INTATTR_BYTE_OFFSET:
        bytes[i] = clicintattr[index].all;
        break;
      case UCLIC_INTCTL_BYTE_OFFSET:
        bytes[i] = clicintctl[index];
        break;
      default:
        return false;
        break;
      }
    }
    return true;
  } else {
    return false;
  }
}

bool clic_t::store(reg_t addr, size_t len, const uint8_t* bytes)  {
if (len > 8) {
    return false;
  }

  tick(0);

  if ((addr >= MCLIC_SMCLICCONFIG_EXT_OFFSET) && (addr < MCLIC_RESERVED1_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    // FIXME add cliccfg register when extension smclicconfig is enabled
    return false;
  } else if ((addr >= MCLIC_RESERVED1_BASE_OFFSET) && (addr < MCLIC_INTTRIG_ADDR_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    return false;
  } else if ((addr >= MCLIC_INTTRIG_ADDR_BASE_OFFSET) && (addr < MCLIC_RESEVED2_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    /* store to clic interrupt trigger memory mapped registers */
    int index = addr - MCLIC_INTTRIG_ADDR_BASE_OFFSET;
    for (int idx = 0; idx < len; idx++)
    {
      // assuming that address is 32 bit aligned
      clicinttrig[index].all = (clicinttrig[index].all & ~((uint32_t(0xFF) << (idx * 8)))) |
                               (uint32_t(bytes[idx]) << (idx * 8));
    }
    return true;
  } else if ((addr >= MCLIC_CUSTOM_BASE_OFFSET ) && (addr < MCLIC_INTTBL_ADDR_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    return false;
  } else if ((addr >= MCLIC_INTTBL_ADDR_BASE_OFFSET ) && (addr < (MCLIC_INTTBL_ADDR_TOP_OFFSET + 4))) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    /* store to clic interrupt table memory mapped registers */
    int index = ((addr & 0XFFFFFFFFFFFFFFFC) - MCLIC_INTTBL_ADDR_BASE_OFFSET) / 4;
    int byte_offset = addr & 0x3;
    for (int idx = 0; idx < len; idx++)
    {
      switch (idx + byte_offset)
      {
      case MCLIC_INTIP_BYTE_OFFSET:
        clicintip[index] = bytes[idx]; // check
        break;
      case MCLIC_INTIE_BYTE_OFFSET:
        clicintie[index] = bytes[idx];
        break;
      case MCLIC_INTATTR_BYTE_OFFSET:
        if (SMCLICSHV_enabled)
        {
        clicintattr[index].all = bytes[idx];
        } else
        {
          clicintattr[index].all = bytes[idx] & ~uint8_t(1);
        }
        break;
      case MCLIC_INTCTL_BYTE_OFFSET:
        clicintctl[index] = bytes[idx];
        break;
      default:
        break;
      }
    }
    return true;
  } else if ((addr >= UCLIC_SMCLICCONFIG_EXT_OFFSET) && (addr < UCLIC_RESERVED_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    // FIXME add cliccfg register when extension smclicconfig is enabled
    return false;
  } else if ((addr >= UCLIC_RESERVED_BASE_OFFSET) && (addr < UCLIC_CUSTOM_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    return false;
  } else if ((addr >= UCLIC_CUSTOM_BASE_OFFSET ) && (addr < UCLIC_INTTBL_ADDR_BASE_OFFSET)) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    return false;
  } else if ((addr >= UCLIC_INTTBL_ADDR_BASE_OFFSET ) && (addr < (UCLIC_INTTBL_ADDR_TOP_OFFSET + 4))) {
    if (len == 8) {
      // Implement double-word stores as a pair of word stores
      return store(addr, 4, bytes) && store(addr + 4, 4, bytes + 4);
    }
    /* store to clic interrupt table memory mapped registers */
    int index = ((addr & 0XFFFFFFFFFFFFFFFC) - UCLIC_INTTBL_ADDR_BASE_OFFSET) / 4;
    int byte_offset = addr & 0x3;
    for (int idx = byte_offset; idx < len; idx++)
    {
      switch (idx)
      {
      case UCLIC_INTIP_BYTE_OFFSET:
        clicintip[index] = bytes[idx]; // check
        break;
      case UCLIC_INTIE_BYTE_OFFSET:
        clicintie[index] = bytes[idx];
        break;
      case UCLIC_INTATTR_BYTE_OFFSET:
        if (SMCLICSHV_enabled)
        {
          clicintattr[index].all = bytes[idx];
        } else
        {
          clicintattr[index].all = bytes[idx] & ~uint8_t(1);
        }
        break;
      case UCLIC_INTCTL_BYTE_OFFSET:
        clicintctl[index] = bytes[idx];
        break;
      default:
        break;
      }
    }
    return true;
  } else {
    return false;
  }
  return true;
}

void clic_t::update_clic_nint() {
  // perform search for higest ranked interrupt in CLIC;
  uint16_t int_selected = 0;
  clic_npriv  = PRV_U;
  clic_nlevel = 0;
  clic_id     = 0;

  for (int indx = 0; indx < CLIC_NUM_INTERRUPT; indx++) {
    if (clicintip[indx] != 0) {
        if (((clicintattr[indx].mode << 8) | clicintctl[indx]) >= int_selected)
        {
        int_selected = ((clicintattr[indx].mode << 8) | clicintctl[indx]);
        clic_npriv  = clicintattr[indx].mode;
        clic_nlevel = clicintctl[indx];
        clic_id     = indx;
        }
    }
  }


};

void clic_t::take_clic_interrupt() {
    state_t* state;
    state = p->get_state();

  // CLIC, version 0.9 draft, 03/03/2024
  //  Current  |      CLIC          |->      Current'          Previous
  //  p/ie/il  | priv   level   id  |->    p/ie/il  pc  cde   pp/il/ie epc
  //  P  ?  ?  | nP<P     ?      ?  |->    - -  -   -   -     -  -  -  -   # Interrupt ignored
  //  P  0  ?  | nP=P     ?      ?  |->    - -  -   -   -     -  -  -  -   # Interrupts disabled
  //  P  1  ?  | nP=P     0      ?  |->    - -  -   -   -     -  -  -  -   # No interrupt
  //  P  1  L  | nP=P   0<nL<=L  ?  |->    - -  -   -   -     -  -  -  -   # Interrupt ignored
  //  P  1  L  | nP=P   L<nL    id  |->    P 0  nL  V   id    P  L  1  pc  # Horizontal interrupt taken
  //  P  ?  ?  | nP>P     0      ?  |->    - -  -   -   -     -  -  -  -   # No interrupt
  //  P  e  L  | nP>P   0<nL    id  |->   nP 0  nL  V   id    P  L  e  pc  # Vertical interrupt taken

// To select an interrupt to present to the core, the CLIC hardware combines the valid bits in clicintattr.mode
// and  clicintctl  to  form  an  unsigned  integer,  then  picks  the  global  maximum  across  all  pending-and-enabled
// interrupts based on this value. Next, the smclicconfig extension defines how to split the clicintctl value into
// interrupt level and interrupt priority. Finally, the interrupt level of this selected interrupt is compared with the
// interrupt-level threshold of the associated privilege mode to determine whether it is qualified or masked by the
// threshold (and thus no interrupt is presented).

  update_clic_nint();

  if ((clic_nlevel == 0) )
  {
    return;
  }
  
  curr_priv = state->prv;
  bool xstatus_xie = false;
  curr_ie = false;
  switch (curr_priv)
  {
  case PRV_U:
    xstatus_xie = (state->ustatus->read() & USTATUS_UIE) ? true : false;
    curr_level = (state->csrmap[CSR_MINTSTATUS]->read() & MINTSTATUS_UIL) >> MINTSTATUS_UIL_LSB;
    if ((curr_level) < (state->csrmap[CSR_UINTTHRESH]->read()))
    {
      curr_level = state->csrmap[CSR_UINTTHRESH]->read();
    }
    break;

  case PRV_S:
    xstatus_xie = (state->sstatus->read() & SSTATUS_SIE) ? true : false;
    curr_level = (state->csrmap[CSR_MINTSTATUS]->read() & MINTSTATUS_SIL) >> MINTSTATUS_SIL_LSB;
    // if (curr_level < state->csrmap[CSR_SINTTHRESH]->read())
    // {
    //   curr_level = state->csrmap[CSR_SINTTHRESH]->read()
    // }
    break;

  case PRV_M:
    xstatus_xie = (state->mstatus->read() & MSTATUS_MIE) ? true : false;
    curr_level = (state->csrmap[CSR_MINTSTATUS]->read() & MINTSTATUS_MIL) >> MINTSTATUS_MIL_LSB;
    if (curr_level < state->csrmap[CSR_MINTTHRESH]->read())
    {
      curr_level = state->csrmap[CSR_MINTTHRESH]->read();
    }
    break;

  default:
    break;
  } 
  
  if (clicintie[clic_id] != 0) {
    p->in_wfi = false;
  }

  curr_ie = xstatus_xie && (clicintie[clic_id] != 0);

  if ((clic_npriv > curr_priv) && (0 < clic_nlevel) && curr_ie) // vertical interrupt
  {
    clic_vrtcl_or_hrzntl_int = true;
  }
  else if ((clic_npriv == curr_priv) && (curr_level < clic_nlevel) && curr_ie) // horizontal interrupt
  {
    clic_vrtcl_or_hrzntl_int = false;
  }
  else // no interrupt
  {
    return;
  }
  
  
 if (p->check_triggers_icount) p->TM.detect_icount_match();
 throw trap_t(((reg_t)1 << (p->isa->get_max_xlen() - 1)) | clic_id);

}

void clic_t::take_clic_trap(trap_t& t, reg_t epc) {
  unsigned max_xlen = p->isa->get_max_xlen();

  // fixme - do the preexisting "debug" and/or "state.debug_mode" mode stuff need to be copied from CLINT mode ?

  reg_t bit = t.cause();
  bool curr_virt = p->state.v;
    const reg_t interrupt_bit = (reg_t)1 << (max_xlen - 1);
  bool interrupt = (bit & interrupt_bit) != 0;

  if (interrupt) {
    bit &= ~((reg_t)1 << (max_xlen - 1));
  } else {
    // fixme - code to handle exception delegation goes here
  }

  reg_t trap_handler_address;
  prev_priv = curr_priv;
  prev_ie   = curr_ie;
  prev_level = curr_level;
  reg_t cause = t.cause();
  cause = set_field(cause,MCAUSE_MPIL, prev_level);
  reg_t  xintstatus = p->state.csrmap[CSR_MINTSTATUS]->read();
  reg_t s;  
  switch (p->CLIC.clic_npriv)
  {
  case PRV_U:
    if (clicintattr[clic_id].shv)
    {
      xlate_flags_t my_xlate_flags = {0,0,0,0};
      reg_t utvt_val = p->state.csrmap[CSR_UTVT]->read();
      auto xlen = p->isa->get_max_xlen();
      reg_t utvt_val_offset = utvt_val + xlen/8*(cause & (reg_t)0xFFF);
      if (xlen == 32)
      {
        trap_handler_address = p->get_mmu()->load<uint32_t>(utvt_val_offset,my_xlate_flags);
      }
      else
      {
        trap_handler_address = p->get_mmu()->load<uint64_t>(utvt_val_offset,my_xlate_flags);
      }
      if (clicintattr[clic_id].trig & (uint8_t)0x1)
      {
        clicintip[clic_id] = 0;
      }
    }
    else
    {
      trap_handler_address = (p->state.utvec->read() & ~(reg_t)63);
    }
    
    p->state.uepc->write(epc);
    xintstatus = set_field(xintstatus, MINTSTATUS_UIL, p->CLIC.clic_nlevel);
    p->state.csrmap[CSR_MINTSTATUS]->write(xintstatus);
    p->state.ucause->write(cause);
    p->state.utval->write(t.get_tval());
    s = p->state.ustatus->read();
    s = set_field(s, USTATUS_UPIE, prev_ie);
    s = set_field(s, USTATUS_UIE, 0);
    p->state.ustatus->write(s);
    break;

  case PRV_S:
    break;

  case PRV_M:
    if (clicintattr[clic_id].shv)
    {
      xlate_flags_t my_xlate_flags = {0,0,0,0};
      reg_t mtvt_val = p->state.csrmap[CSR_MTVT]->read();
      auto xlen = p->isa->get_max_xlen();
      reg_t mtvt_val_offset = mtvt_val + xlen/8*(cause & (reg_t)0xFFF);
      if (xlen == 32)
      {
        trap_handler_address = p->get_mmu()->load<uint32_t>(mtvt_val_offset,my_xlate_flags);
      }
      else
      {
        trap_handler_address = p->get_mmu()->load<uint64_t>(mtvt_val_offset,my_xlate_flags);
      }
      if (clicintattr[clic_id].trig & (uint8_t)0x1)
      {
        clicintip[clic_id] = 0;
      }
      
    }
    else
    {
    trap_handler_address = (p->state.mtvec->read() & ~(reg_t)63);
    }
    
    p->state.mepc->write(epc);
    xintstatus = set_field(xintstatus, MINTSTATUS_MIL, p->CLIC.clic_nlevel);
    p->state.csrmap[CSR_MINTSTATUS]->write(xintstatus);
    p->state.mcause->write(cause);
    p->state.mtval->write(t.get_tval());
    p->state.mtval2->write(t.get_tval2());
    p->state.mtinst->write(t.get_tinst());
    s = p->state.mstatus->read();
    s = set_field(s, MSTATUS_MPIE, prev_ie);
    s = set_field(s, MSTATUS_MPP, prev_priv);
    s = set_field(s, MSTATUS_MIE, 0);
    s = set_field(s, MSTATUS_MPV, curr_virt);
    s = set_field(s, MSTATUS_GVA, t.has_gva());
    s = set_field(s, MSTATUS_MPELP, p->state.elp);
    p->state.mstatus->write(s);
    if (p->state.mstatush) p->state.mstatush->write(s >> 32);  // log mstatush change
    break;

  default:
    break;
  } 

  const reg_t rnmi_trap_handler_address = 0;
  const bool nmie = !(p->state.mnstatus && !get_field(p->state.mnstatus->read(), MNSTATUS_NMIE));
  
  p->state.pc = !nmie ? rnmi_trap_handler_address : trap_handler_address;
  
  p->state.elp = elp_t::NO_LP_EXPECTED;

  switch (p->CLIC.clic_npriv)
  {
  case PRV_U:
    p->set_privilege(PRV_U, false);
    break;
  case PRV_M:
    p->set_privilege(PRV_M, false);
    break;
  
  default:
    break;
  }

}

void clic_t::reset() {
  auto& csrmap = p->get_state()->csrmap;
  csrmap[CSR_MTVT]  = std::make_shared<tvt_t>(p,CSR_MTVT);
  csrmap[CSR_MNXTI] = std::make_shared<nxti_t>(p, CSR_MNXTI);
  csrmap[CSR_MINTSTATUS] = std::make_shared<intstatus_t>(p, CSR_MINTSTATUS);
  csrmap[CSR_MINTTHRESH] = std::make_shared<intthresh_t>(p, CSR_MINTTHRESH);
  csrmap[CSR_MSCRATCHCSW] = std::make_shared<scratchcsw_t>(p, CSR_MSCRATCHCSW);
  csrmap[CSR_MSCRATCHCSWL] = std::make_shared<scratchcswl_t>(p, CSR_MSCRATCHCSWL);

  csrmap[CSR_UTVT]  = std::make_shared<tvt_t>(p,CSR_UTVT);
  csrmap[CSR_UNXTI] = std::make_shared<nxti_t>(p, CSR_UNXTI);
  csrmap[CSR_UINTSTATUS] = std::make_shared<intstatus_t>(p, CSR_UINTSTATUS);
  csrmap[CSR_UINTTHRESH] = std::make_shared<intthresh_t>(p, CSR_UINTTHRESH);
  csrmap[CSR_USCRATCHCSWL] = std::make_shared<scratchcswl_t>(p, CSR_USCRATCHCSWL);

  reg_t mintstatus = csrmap[CSR_MINTSTATUS]->read();
  mintstatus = mintstatus & ~MINTSTATUS_MIL;
  mintstatus = mintstatus & ~MINTSTATUS_UIL;
  csrmap[CSR_MINTSTATUS]->write(mintstatus);
  reg_t mcause = csrmap[CSR_MCAUSE]->read();
  mcause = mcause & ~MCAUSE_MPIL;
  csrmap[CSR_MCAUSE]->write(mcause);

  curr_priv = PRV_U;
  prev_priv = PRV_U;
  curr_level = 0;
  prev_level = 0;
  curr_ie = 0;
  prev_ie = 0;
  clic_npriv = PRV_U;
  clic_nlevel = 0;
  clic_id = 0;

}

clic_t* clic_parse_from_fdt(const void* fdt, const sim_t* sim, reg_t* base,
    const std::vector<std::string>& sargs) {
  if (fdt_parse_clic(fdt, base, "riscv,clic0") == 0 || fdt_parse_clic(fdt, base, "sifive,clic0") == 0)
    return new clic_t();
  else
    return nullptr;
}

std::string clic_generate_dts(const sim_t* sim,  const std::vector<std::string>& UNUSED sargs) {
  std::stringstream s;
  s << std::hex
    << "    clic@" << MCLIC_BASE << " {\n"
       "      compatible = \"riscv,clic0\";\n"
       "      interrupts-extended = <" << std::dec;
  for (size_t i = 0; i < sim->get_cfg().nprocs(); i++)
    s << "&CPU" << i << "_intc 3 &CPU" << i << "_intc 7 ";
  reg_t mclicbs = MCLIC_BASE;
  reg_t mclicsz = MCLIC_SIZE;
  reg_t uclicbs = UCLIC_BASE;
  reg_t uclicsz = UCLIC_SIZE;
  s << std::hex << ">;\n"
    "      reg = <0x" << (mclicbs >> 32) << " 0x" << (mclicbs & (uint32_t)-1) << " 0x" << (mclicsz >> 32) << " 0x" << (mclicsz & (uint32_t)-1)
              << " 0x" << (uclicbs >> 32) << " 0x" << (uclicbs & (uint32_t)-1) << " 0x" << (uclicsz >> 32) << " 0x" << (uclicsz & (uint32_t)-1) << ">;\n"
    "    };\n";
  return s.str();
}

REGISTER_DEVICE(clic, clic_parse_from_fdt, clic_generate_dts)

clic_t::clic_t(/* args */) :
  p(0),
  sim(0) {
}

clic_t::~clic_t()
{
}
