#ifdef CX4_CPP

unsigned Cx4::speed(unsigned addr) {
  if ((addr & 0xf08000) == 0x700000) { // cart RAM
    return mmio.ramSpeed;
  } else if ((addr & 0x408000) == 0x008000) { // cart ROM
    return mmio.romSpeed;
  }
  
  // internal / unmapped
  return 0;
}

uint8 Cx4::read(unsigned addr) {
  if (!Memory::debugger_access())
    active() ? synchronize_cpu() : cpu.synchronize_coprocessor();

  if((addr & 0x0c00) == 0x0c00) {  //$00-3f,80-bf:6c00-6cff,7c00-7cff
    return dsp_read(addr);
  }
  if((addr & 0x0c00) < 0x0c00) {  //$00-3f,80-bf:6000-6bff,7000-7bff
    return dram_read(addr);
  }
  
  return 0;
}

void Cx4::write(unsigned addr, uint8 data) {
  if (!Memory::debugger_access())
    active() ? synchronize_cpu() : cpu.synchronize_coprocessor();

  if((addr & 0x0c00) == 0x0c00) {  //$00-3f,80-bf:6c00-6cff,7c00-7cff
    return dsp_write(addr, data);
  }
  if((addr & 0x0c00) < 0x0c00) {  //$00-3f,80-bf:6000-6bff,7000-7bff
    return dram_write(addr, data);
  }
}

uint8 Cx4::rom_read(unsigned addr) {
  if (active() || mmio.suspend || !busy()) {
    return memory::cartrom.read(addr);
  }
  
  if ((addr & 0xffffe0) == 0x007fe0) return mmio.vector[addr & 0x1f];
  return 0; // not open bus
}

uint8 Cx4::dram_read(unsigned addr) {
  addr &= 0xfff;
  if(addr >= 0xc00) return 0;
  return dataRAM[addr];
}

void Cx4::dram_write(unsigned addr, uint8 data) {
  addr &= 0xfff;
  if(addr >= 0xc00) return;
  dataRAM[addr] = data;
}

uint8 Cx4::dsp_read(unsigned addr) {
  addr = 0x7c00 | (addr & 0x03ff);

  //MMIO
  switch(addr) {
  case 0x7f40: return mmio.dmaSource >>  0;
  case 0x7f41: return mmio.dmaSource >>  8;
  case 0x7f42: return mmio.dmaSource >> 16;
  case 0x7f43: return mmio.dmaLength >>  0;
  case 0x7f44: return mmio.dmaLength >>  8;
  case 0x7f45: return mmio.dmaTarget >>  0;
  case 0x7f46: return mmio.dmaTarget >>  8;
  case 0x7f47: return mmio.dmaTarget >> 16;
  case 0x7f48: return mmio.cachePreload;
  case 0x7f49: return mmio.programOffset >>  0;
  case 0x7f4a: return mmio.programOffset >>  8;
  case 0x7f4b: return mmio.programOffset >> 16;
  case 0x7f4c: return (cache[1].lock << 1) | cache[0].lock;
  case 0x7f4d: return mmio.pageNumber >> 0;
  case 0x7f4e: return mmio.pageNumber >> 8;
  case 0x7f4f: return mmio.programCounter;
  case 0x7f50: return (mmio.romSpeed << 4) | mmio.ramSpeed;
  case 0x7f51: return mmio.r1f51;
  case 0x7f52: return mmio.r1f52;
  case 0x7f53: case 0x7f54: case 0x7f55: case 0x7f56:
  case 0x7f57: case 0x7f59: 
  case 0x7f5b: case 0x7f5c: case 0x7f5d: case 0x7f5e:
  case 0x7f5f: return (busy() << 7) | (running() << 6) | (regs.halt << 1) | mmio.suspend;
  }

  //Vector
  if(addr >= 0x7f60 && addr <= 0x7f7f) {
    return mmio.vector[addr & 0x1f];
  }

  //GPRs
  if((addr >= 0x7f80 && addr <= 0x7faf) || (addr >= 0x7fc0 && addr <= 0x7fef)) {
    unsigned index = (addr & 0x3f) / 3;        //0..15
    unsigned shift = ((addr & 0x3f) % 3) * 8;  //0, 8, 16
    return regs.gpr[index] >> shift;
  }

  return 0x00;
}

void Cx4::dsp_write(unsigned addr, uint8 data) {
  addr = 0x7c00 | (addr & 0x03ff);

  //MMIO
  switch(addr) {
  case 0x7f40: mmio.dmaSource = (mmio.dmaSource & 0xffff00) | (data <<  0); return;
  case 0x7f41: mmio.dmaSource = (mmio.dmaSource & 0xff00ff) | (data <<  8); return;
  case 0x7f42: mmio.dmaSource = (mmio.dmaSource & 0x00ffff) | (data << 16); return;
  case 0x7f43: mmio.dmaLength = (mmio.dmaLength &   0xff00) | (data <<  0); return;
  case 0x7f44: mmio.dmaLength = (mmio.dmaLength &   0x00ff) | (data <<  8); return;
  case 0x7f45: mmio.dmaTarget = (mmio.dmaTarget & 0xffff00) | (data <<  0); return;
  case 0x7f46: mmio.dmaTarget = (mmio.dmaTarget & 0xff00ff) | (data <<  8); return;
  case 0x7f47: mmio.dmaTarget = (mmio.dmaTarget & 0x00ffff) | (data << 16);
    if(regs.halt) mmio.dma = true;
    return;
  case 0x7f48: 
    if(regs.halt) {
      mmio.cachePreload = data & 0x01; 
      mmio.cacheLoading = true;
    }
    return;
  case 0x7f49: mmio.programOffset = (mmio.programOffset & 0xffff00) | (data <<  0); return;
  case 0x7f4a: mmio.programOffset = (mmio.programOffset & 0xff00ff) | (data <<  8); return;
  case 0x7f4b: mmio.programOffset = (mmio.programOffset & 0x00ffff) | (data << 16); return;
  case 0x7f4c: 
    if(regs.halt) {
      cache[0].lock = data & 1;
      cache[1].lock = data & 2;
    }
    return;
  case 0x7f4d: mmio.pageNumber = (mmio.pageNumber & 0x7f00) | ((data & 0xff) << 0); return;
  case 0x7f4e: mmio.pageNumber = (mmio.pageNumber & 0x00ff) | ((data & 0x7f) << 8); return;
  case 0x7f4f: mmio.programCounter = data;
    if(regs.halt) {
      regs.pc = mmio.pageNumber * 256 + mmio.programCounter;
      regs.halt = false;
      regs.cachePage = 0;
    }
    return;
  case 0x7f50:
    mmio.romSpeed = (data >> 4) & 0x7;
    mmio.ramSpeed = (data >> 0) & 0x7;
    return;
  case 0x7f51: mmio.r1f51 = data & 0x01; return;
  case 0x7f52: mmio.r1f52 = data & 0x01; return;
  case 0x7f53: regs.halt = true; return;
  case 0x7f55: case 0x7f56: case 0x7f57: case 0x7f58: 
  case 0x7f59: case 0x7f5a: case 0x7f5b: case 0x7f5c: 
    mmio.suspend = true;
    mmio.suspendCycles = 32 * (addr - 0x7f55);
    return;
  case 0x7f5d: mmio.suspend = false; return;
  }

  //Vector
  if(addr >= 0x7f60 && addr <= 0x7f7f) {
    mmio.vector[addr & 0x1f] = data;
    return;
  }

  //GPRs
  if((addr >= 0x7f80 && addr <= 0x7faf) || (addr >= 0x7fc0 && addr <= 0x7fef)) {
    unsigned index = (addr & 0x3f) / 3;
    switch((addr & 0x3f) % 3) {
    case 0: regs.gpr[index] = (regs.gpr[index] & 0xffff00) | (data <<  0); return;
    case 1: regs.gpr[index] = (regs.gpr[index] & 0xff00ff) | (data <<  8); return;
    case 2: regs.gpr[index] = (regs.gpr[index] & 0x00ffff) | (data << 16); return;
    }
  }
}

#endif

