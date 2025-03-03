/////////////////////////////////////////////////////////////////////////////////////////
//
// PIO simulator
//
// ADAPTED FROM: https://github.com/sumio-morioka/rpipico_simple_PIO_emulator
//   Full credit to Sumio Morioka for doing the hard work.
/////////////////////////////////////////////////////////////////////////////////////////

#include "ArgsParser.h"

#define	PIO_OUT_GPIO_BIT_BY_BIT
#undef	PIO_IN_GPIO_BIT_BY_BIT
#include "picopio_emu.h"

#include <fstream>
#include <iostream>

////////////////

struct Args
{
	bool help = false;

	std::string progHex;
	std::string inCsv = "in.csv";
	std::string outCsv = "out.csv";
	std::string traceOut;
	std::string asmOut;

	unsigned nCycles = 0;
	bool step = false;

	unsigned    iWrapAt = PIO_UNUSE;
	unsigned    iWrapTo = PIO_UNUSE;

	unsigned	iInPins = PIO_UNUSE;
	unsigned	nInPins = PIO_UNUSE;
	unsigned	iOutPins = PIO_UNUSE;
	unsigned	nOutPins = PIO_UNUSE;
	unsigned	iSidePins = PIO_UNUSE;
	unsigned	nSidePins = PIO_UNUSE;
	bool		sideOpt = false;

	std::string	isrShiftDir = "L";
	unsigned	isrAutopushThreshold = 0;  // 0=disabled, else 1-32
	std::string	osrShiftDir = "L";
	unsigned	osrAutopullThreshold = 0;  // 0=disabled, else 1-32
	unsigned	iJmpPin = PIO_UNUSE;
	bool		movStatusTx = false;
	unsigned	movStatusN = 4;

private:
	std::string m_badArg;

public:
	bool Parse(int argc, char* argv[])
	{
		ArgsParser parser(argc, argv);
		while (!parser.Done())
		{
			if (parser.ParseFlag(help, { "-h", "--help" })) continue;
				
			if (parser.ParseString(progHex,  { "--hex" }))continue;
			if (parser.ParseString(inCsv,    { "--in" })) continue;
			if (parser.ParseString(outCsv,   { "--out" })) continue;
			if (parser.ParseString(traceOut, { "--trace" })) continue;
			if (parser.ParseString(asmOut,   { "--asm" })) continue;

			if (parser.ParseUint(nCycles, { "--cycles" })) continue;
			if (parser.ParseFlag(step,    { "--step" })) continue;

			if (parser.ParseUint(iWrapAt, { "--iWrapAt" })) continue;
			if (parser.ParseUint(iWrapTo, { "--iWrapTo" })) continue;

			if (parser.ParseUint(iInPins,   { "--iIn" })) continue;
			if (parser.ParseUint(nInPins,   { "--nIn" })) continue;
			if (parser.ParseUint(iOutPins,  { "--iOut" })) continue;
			if (parser.ParseUint(nOutPins,  { "--nOut" })) continue;
			if (parser.ParseUint(iSidePins, { "--iSide" })) continue;
			if (parser.ParseUint(nSidePins, { "--nSide" })) continue;
			if (parser.ParseFlag(sideOpt,   { "--sideOpt" })) continue;

			if (parser.ParseString(isrShiftDir,        { "--isrDir" }, { "L", "l", "R", "r" })) continue;
			if (parser.ParseUint(isrAutopushThreshold, { "--autopush" })) continue;
			if (parser.ParseString(osrShiftDir,        { "--osrDir" }, { "L", "l", "R", "r" })) continue;
			if (parser.ParseUint(osrAutopullThreshold, { "--autopull" })) continue;
			if (parser.ParseUint(iJmpPin,              { "--jmpPin" })) continue;
			if (parser.ParseFlag(movStatusTx,          { "--statusTx" })) continue;
			if (parser.ParseUint(movStatusN,           { "--statusN" })) continue;

			m_badArg = parser.NextArg();
			return false;
		}
		return true;
	}
	
	void Help(std::ostream& out)
	{
		out << "piosim - PIO simulator" << "\n";
		out << "\n";
		out << "    --hex <filename>   - Program hex file" << "\n";
		out << "    --in <filename>    - Input csv file" << "\n";
		out << "    --out <filename>   - Output csv file" << "\n";
		out << "    --trace <filename> - Trace output file" << "\n";
		out << "    --asm <filename>   - Asm output file" << "\n";
		out << "\n";
		out << "    --cycles <n>       - Simulation cycle count" << "\n";
		out << "    --step             - Simulate in single steps" << "\n";
		out << "\n";
		out << "    --iIn <n>          - First input pin" << "\n";
		out << "    --nIn <n>          - Num input pins" << "\n";
		out << "    --iOut <n>         - First output pin" << "\n";
		out << "    --nOut <n>         - Num output pins" << "\n";
		out << "    --iSide <n>        - First sideset pin" << "\n";
		out << "    --nSide <n>        - Num sideset pins" << "\n";
		out << "    --sideOpt          - Use optional sideset" << "\n";
		out << "\n";
		out << "    --isrDir <L|R>     - ISR shift dir" << "\n";
		out << "    --autopush <n>     - Enable autopush with specified threshold" << "\n";
		out << "    --osrDir <L|R>     - OSR shift dir" << "\n";
		out << "    --autopull <n>     - Enable autopull with specified threshold" << "\n";
		out << "    --jmpPin <n>       - Emulation cycle count" << "\n";
		out << "    --statusTx         - Emulation cycle count" << "\n";
		out << "    --statusN          - Emulation cycle count" << "\n";
		out << "\n";
	}

	std::string ErrorMsg() const
	{
		return std::string("Bad arg: ") + m_badArg;
	}
};

////////////////

struct Files
{
	std::istream* fprog = nullptr;
	FILE* fin = nullptr;
	FILE* fout = nullptr;
	FILE* ftrace = nullptr;
	FILE* fasm = nullptr;

	void Open(const Args& i_args)
	{
		fprog = OpenIn(m_ifprog, i_args.progHex, "program hex");
		fin = OpenIn(i_args.inCsv, "input csv");
		fout = OpenOut(i_args.outCsv, "output csv");
		if (!i_args.traceOut.empty())
			ftrace = OpenOut(i_args.traceOut, "trace output");
		if (!i_args.asmOut.empty())
			fasm = OpenOut(i_args.asmOut, "asm output");
	}

private:
	std::ifstream m_ifprog;	

private:
	std::istream* OpenIn(std::ifstream& i_ifstream, const std::string& i_name, const std::string& i_role)
	{
		if (i_name == "stdin")
			return &std::cin;
		i_ifstream.open(i_name);
		if (!i_ifstream)
		{
			std::cerr << "pioemu: ERROR. Cannot open " << i_role << " file '" << i_name << "'\n";
			exit(1);
		}
		return &i_ifstream;
	}

	FILE* OpenIn(const std::string& i_name, const std::string& i_role)
	{
		if (i_name == "stdin")
			return stdin;
		FILE* f;
		if ((f = fopen(i_name.c_str(), "rt")) == nullptr)
		{
			std::cerr << "pioemu: ERROR. Cannot open " << i_role << " file '" << i_name << "'\n";
			exit(1);
		}
		return f;
	}

	FILE* OpenOut(const std::string& i_name, const std::string& i_role)
	{
		if (i_name == "stdout")
			return stdout;
		if (i_name == "stderr")
			return stderr;
		FILE* f;
		if ((f = fopen(i_name.c_str(), "wt")) == nullptr)
		{
			std::cerr << "pioemu: ERROR. Cannot open " << i_role << " file '" << i_name << "'\n";
			exit(1);
		}
		return f;
	}
};

////////////////

static unsigned ExtractOpcode(unsigned i_word)
{
	return (i_word >> 13) & 0x7;
}

static unsigned ExtractDelay(unsigned i_word, unsigned i_nSide, bool i_sideOpt)
{
	// LSBs
	unsigned nDelay = (5 - i_sideOpt) - i_nSide;
	return (i_word >> 8) & pio_maxval(nDelay);
}

static unsigned ExtractSideset(unsigned i_word, unsigned i_nSide, bool i_sideOpt)
{
	// MSBs
	if (i_sideOpt && !((i_word >> 12) & 0x1))
		return PIO_UNUSE;
	unsigned nDelay = (5 - i_sideOpt) - i_nSide;
	return (i_word >> (8 + nDelay)) & pio_maxval(i_nSide);
}

static unsigned Cond(unsigned i_field)
{
	switch (i_field)
	{
	case 0: return PIO_ALWAYS;
	case 1: return PIO_X_EQ_0;
	case 2: return PIO_X_NEQ_0_DEC;
	case 3: return PIO_Y_EQ_0;
	case 4: return PIO_Y_NEQ_0_DEC;
	case 5: return PIO_X_NEQ_Y;
	case 6: return PIO_PIN;
	case 7: return PIO_OSRE_NOTEMPTY;
	default: return 0;
	}
}

static unsigned WaitSrc(unsigned i_field)
{
	switch (i_field)
	{
	case 0: return PIO_GPIO;
	case 1: return PIO_PIN;
	case 2: return PIO_IRQ;
	default: return 0;
	}
}

static unsigned InSrc(unsigned i_field)
{
	switch (i_field)
	{
	case 0: return PIO_PINS;
	case 1: return PIO_X;
	case 2: return PIO_Y;
	case 3: return PIO_NULL;
	case 6: return PIO_ISR;
	case 7: return PIO_OSR;
	default: return 0;
	}
}

static unsigned OutDst(unsigned i_field)
{
	switch (i_field)
	{
	case 0: return PIO_PINS;
	case 1: return PIO_X;
	case 2: return PIO_Y;
	case 3: return PIO_NULL;
	case 4: return PIO_PINDIRS;
	case 5: return PIO_PC;
	case 6: return PIO_ISR;
	case 7: return PIO_EXEC;
	default: return 0;
	}
}

static unsigned BitCount(unsigned i_field)
{
	return (i_field == 0) ? 32 : i_field;
}

static unsigned Block(unsigned i_field)
{
	return i_field ? PIO_BLOCK : PIO_NOBLOCK;
}

static unsigned MovSrc(unsigned i_field)
{
	switch (i_field)
	{
	case 0: return PIO_PINS;
	case 1: return PIO_X;
	case 2: return PIO_Y;
	case 3: return PIO_NULL;
	// 4 n/a
	case 5: return PIO_STATUS;
	case 6: return PIO_ISR;
	case 7: return PIO_OSR;
	default: return 0;
	}
}

static unsigned MovDst(unsigned i_field)
{
	switch (i_field)
	{
	case 0: return PIO_PINS;
	case 1: return PIO_X;
	case 2: return PIO_Y;
	// 3 n/a
	case 4: return PIO_EXEC;
	case 5: return PIO_PC;
	case 6: return PIO_ISR;
	case 7: return PIO_OSR;
	default: return 0;
	}
}

static unsigned MovOp(unsigned i_field)
{
	switch (i_field)
	{
	case 0: return PIO_NONE;
	case 1: return PIO_INVERT;
	case 2: return PIO_BIT_REVERSE;
	default: return 0;
	}
}

static unsigned SetDst(unsigned i_field)
{
	switch (i_field)
	{
	case 0: return PIO_PINS;
	case 1: return PIO_X;
	case 2: return PIO_Y;
	// 3 n/a
	case 4: return PIO_PINDIRS;
	default: return 0;
	}
}

static void ParseInstruction(unsigned i_word, const Args& i_args)
{
	unsigned opcode = ExtractOpcode(i_word);
	unsigned delay = ExtractDelay(i_word, i_args.nSidePins, i_args.sideOpt);
	unsigned side = ExtractSideset(i_word, i_args.nSidePins, i_args.sideOpt);

	switch (opcode)
	{
	case 0: // JMP
	{
		unsigned cond = Cond((i_word >> 5) & 0x7);
		unsigned addr = i_word & 0x1f;
		char label[10];
		sprintf(label, "@0x%02x", addr);
		return pio_jmp(cond, label, side, delay);
	}
	case 1: // WAIT
	{
		unsigned pol = (i_word >> 7) & 0x1;
		unsigned src = WaitSrc((i_word >> 5) & 0x3);
		unsigned rel = (i_word >> 4) & 0x1;
		unsigned index = i_word & 0xf;
		return pio_wait(pol, src, index, rel, side, delay);
	}
	case 2: // IN
	{
		unsigned src = InSrc((i_word >> 5) & 0x7);
		unsigned count = BitCount(i_word & 0x1f);
		return pio_in(src, count, side, delay);
	}
	case 3: // OUT
	{
		unsigned dst = OutDst((i_word >> 5) & 0x7);
		unsigned count = BitCount(i_word & 0x1f);
		return pio_out(dst, count, side, delay);
	}
	case 4: // PUSH, PULL
	{
		unsigned pull = (i_word >> 7) & 0x1;
		unsigned block = Block((i_word >> 5) & 0x1);
		if (pull)
		{
			unsigned ifE = (i_word >> 6) & 0x1;
			return pio_pull(ifE, block, side, delay);
		}
		else
		{
			unsigned ifF = (i_word >> 6) & 0x1;
			return pio_push(ifF, block, side, delay);
		}
	}
	case 5: // MOV
	{
		unsigned src = MovSrc(i_word & 0x7);
		unsigned dst = MovDst((i_word >> 5) & 0x7);
		unsigned op = MovOp((i_word >> 3) & 0x3);
		return pio_mov(dst, op, src, side, delay);
	}
	case 6: // IRQ
	{
		unsigned clr = (i_word >> 6) & 0x1;
		unsigned wait = (i_word >> 5) & 0x1;
		unsigned rel = (i_word >> 4) & 0x1;
		unsigned index = i_word & 0xf;
		return pio_irq(clr, wait, index, rel, side, delay);
	}
	case 7: // SET
	{
		unsigned dst = SetDst((i_word >> 5) & 0x7);
		unsigned data = i_word & 0x1f;
		return pio_set(dst, data, side, delay);
	}
	}
}

////////////////

static bool ParseHex(unsigned& o_instr, const std::string& i_line, unsigned i_iLine, const Args& i_args)
{
	size_t p = 0;
	o_instr = std::stoul(i_line, &p, 16);
	if (p == 0)
		return false;

	if ((o_instr > 0xffff) || (i_line[p] && !std::isspace(i_line[p])))
	{
		std::cerr << "pioemu: ERROR. Invalid value in hex file '" << i_args.progHex << "', line " << i_iLine << ":\n";
		std::cerr << i_line << std::endl;
		exit(1);
	}

	return true;
}

////////////////

int main(int argc, char *argv[])
{
	Args args;
	if (!args.Parse(argc, argv))
	{
		std::cerr << args.ErrorMsg() << std::endl;
		return 1;
	}
	
	if (args.help || (argc == 1))
	{
		args.Help(std::cerr);
		return 0;
	}

	Files files;
	files.Open(args);

	// Prepare
	{
		pio_code_start(
			"pio_prog",
			0,
			args.iInPins,
			args.nInPins,
			args.iOutPins,
			args.nOutPins,
			args.iSidePins,
			args.nSidePins,
			args.sideOpt,
			(args.isrShiftDir == "R" || args.isrShiftDir == "r"),
			(args.isrAutopushThreshold > 0),
			(args.isrAutopushThreshold > 0) ? args.isrAutopushThreshold : 32,
			(args.osrShiftDir == "R" || args.osrShiftDir == "r"),
			(args.osrAutopullThreshold > 0),
			(args.osrAutopullThreshold > 0) ? args.osrAutopullThreshold : 32,
			args.iJmpPin,
			args.movStatusTx,
			args.movStatusN
		);

		if (args.iWrapTo == PIO_UNUSE)
			pio_wrap_target();

		unsigned line = 0;
		std::string s;
		while (std::getline(*files.fprog, s))
		{
			unsigned instr;
			if (ParseHex(instr, s, ++line, args))
			{
				if (args.iWrapTo == line)
					pio_wrap_target();
	
				ParseInstruction(instr, args);

				if (args.iWrapAt == line)
					pio_wrap();
			}
		}

		if (args.iWrapAt == PIO_UNUSE)
			pio_wrap();

		pio_code_end(false, nullptr);
	}

	// Simulate
	{
		int cycles = args.nCycles;

		pio_begin_emulation(
			files.fin,
			files.fout,
			files.ftrace,
			files.fasm
		);

		while (cycles-- > 0)
		{
			if (args.step)
			{
				char buf[100];
				fputs(">", stdout);
				fgets(buf, 100, stdin);
			}
			pio_step_emulation(1);
		}

		pio_end_emulation();
	}

	return 0;
}
