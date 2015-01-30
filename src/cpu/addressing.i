/* Implied */
static void IMP() {
	/* Leave empty */
}

/* Accumulator */
static void ACC() {
	/* Leave empty */
}

/* Immediate */
static void IMM() {
	cpu.operaddr = cpu.PC++;
}

/* Zero-page */
static void ZPG() {
	cpu.operaddr = (WORD)Memory_ReadByte((WORD)cpu.PC++);
}

/* Zero-page, X */
static void ZPX() {

}

/* Zero-page, Y */
static void ZPY() {

}

/* Relative */
static void REL() {
	cpu.operaddr = Memory_ReadByte(cpu.PC);
}

/* Absolute */
static void ABS() {
	cpu.operaddr = Memory_ReadWord(cpu.PC);
	cpu.PC += 2;
}

/* Absolute, X */
static void ABX() {
	cpu.operaddr = cpu.X + Memory_ReadWord(cpu.PC);
	cpu.PC += 2;
}

/* Absolute, Y */
static void ABY() {
	cpu.operaddr = cpu.Y + Memory_ReadWord(cpu.PC);
	cpu.PC += 2;
}

/* Indirect */
static void IND() {
 	cpu.operaddr = Memory_ReadWord(Memory_ReadWord(cpu.PC));
	cpu.PC += 2;
	/* TODO: Implement bug */
}

/* (Indirect, X) */
static void IDX() {
	cpu.operaddr = Memory_ReadWord(cpu.X + Memory_ReadWord(cpu.PC));
	cpu.PC += 2;
}

/* (Indirect), Y */
static void IDY() {
	cpu.operaddr = Memory_ReadWord(Memory_ReadWord(cpu.PC));
	cpu.operaddr += cpu.Y;
	cpu.PC += 2;
}