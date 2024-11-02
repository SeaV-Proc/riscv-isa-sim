require_extension('S');
reg_t prev_hstatus = STATE.hstatus->read();
if (STATE.v) {
  if (STATE.prv == PRV_U || get_field(prev_hstatus, HSTATUS_VTSR))
    require_novirt();
} else {
  require_privilege(get_field(STATE.mstatus->read(), MSTATUS_TSR) ? PRV_M : PRV_S);
}
if (p->extension_enabled(EXT_SMCLICSHV) && (STATE.scause->read() & SCAUSE_INHV)) {
  xlate_flags_t my_xlate_flags = {0,0,0,0};
  reg_t new_sepc = p->get_mmu()->load<uint32_t>(p->get_state()->sepc->read(),my_xlate_flags);
  set_pc_and_serialize(new_sepc);
} else {
  reg_t next_pc = p->get_state()->sepc->read();
  set_pc_and_serialize(next_pc);
}
reg_t s = STATE.sstatus->read();
reg_t prev_prv = get_field(s, MSTATUS_SPP);
s = set_field(s, MSTATUS_SIE, get_field(s, MSTATUS_SPIE));
s = set_field(s, MSTATUS_SPIE, 1);
s = set_field(s, MSTATUS_SPP, PRV_U);
bool prev_virt = STATE.v;
if (!STATE.v) {
  if (p->extension_enabled('H')) {
    prev_virt = get_field(prev_hstatus, HSTATUS_SPV);
    reg_t new_hstatus = set_field(prev_hstatus, HSTATUS_SPV, 0);
    STATE.hstatus->write(new_hstatus);
  }

  STATE.mstatus->write(set_field(STATE.mstatus->read(), MSTATUS_MPRV, 0));
}
if (ZICFILP_xLPE(prev_virt, prev_prv)) {
  STATE.elp = static_cast<elp_t>(get_field(s, SSTATUS_SPELP));
}
s = set_field(s, SSTATUS_SPELP, elp_t::NO_LP_EXPECTED);
STATE.sstatus->write(s);
p->set_privilege(prev_prv, prev_virt);
