#include <ir/ir.h>
#include <target/util.h>

const char* ccmp_str(Inst* inst, const char* true_str) {
  int op = normalize_cond(inst->op, 0);
  const char* op_str;
  switch (op) {
    case JEQ:
      op_str = "="; break;
    case JNE:
      op_str = "!=:"; break;
    case JLT:
      op_str = "<"; break;
    case JGT:
      op_str = ">"; break;
    case JLE:
      op_str = "<=:"; break;
    case JGE:
      op_str = ">=:"; break;
    case JMP:
      return true_str;
    default:
      error("oops");
  }
  return format("%s %s %s", reg_names[inst->dst.reg], op_str, src_str(inst));
}

static const char* ctr_REG_NAMES[] = {
  "a", "b", "c", "d", "bp", "sp", "pc"
};

static void init_state_ctr(Data* data) {
  emit_line("#!/usr/bin/env ctr");
  emit_line("");
  emit_line("#:language XFrozen");
  emit_line("");
  emit_line("Broom memoryLimit: 1024 * 1024 * 1024 * 4.");
  reg_names = ctr_REG_NAMES;
  for (int i = 0; i < 7; i++) {
    emit_line("var %s is 0.", reg_names[i]);
  }
  int mp;
  Data* mdata = data;
  for (mp = 0; mdata; mdata = mdata->next, mp++);
  emit_line("var mem is Array new: " UINT_MAX_STR ", fill: " UINT_MAX_STR " with: 0.");
  /* emit_line("mem on: 'at:' do: {:&x");
  inc_indent();
  emit_line("me count < x ifTrue: { put: 0 at: x. ^0. } ifFalse: { ^`at: x. }.");
  dec_indent();
  emit_line("}."); */
  emit_line("mem do");
  inc_indent();
  for (int mp = 0; data; data = data->next, mp++)
    emit_line("put: %d at: %d,", data->v, mp);
  dec_indent();
  emit_line("done.");
}

static void ctr_emit_inst(Inst* inst) {
  switch (inst->op) {
  case MOV:
    emit_line("%s is %s.", reg_names[inst->dst.reg], src_str(inst));
    break;

  case ADD:
    emit_line("%s is (%s + %s) bitAnd: " UINT_MAX_STR ".",
              reg_names[inst->dst.reg],
              reg_names[inst->dst.reg], src_str(inst));
    break;

  case SUB:
    emit_line("%s is (%s - %s) bitAnd: " UINT_MAX_STR ".",
              reg_names[inst->dst.reg],
              reg_names[inst->dst.reg], src_str(inst));
    break;

  case LOAD:
    emit_line("%s is mem at: %s.", reg_names[inst->dst.reg], src_str(inst));
    break;

  case STORE:
    emit_line("mem put: %s at: %s.", reg_names[inst->dst.reg], src_str(inst));
    break;

  case PUTC:
    emit_line("Pen write: ('' appendByte: %s).", src_str(inst));
    break;

  case GETC:
    emit_line("%s is nextchar run.",
              reg_names[inst->dst.reg]);
    break;

  case EXIT:
    emit_line("Program exit.");
    break;

  case DUMP:
    break;

  case EQ:
  case NE:
  case LT:
  case GT:
  case LE:
  case GE:
    emit_line("%s is (%s) toNumber.",
              reg_names[inst->dst.reg], ccmp_str(inst, "True"));
    break;

  case JEQ:
  case JNE:
  case JLT:
  case JGT:
  case JLE:
  case JGE:
    emit_line("(%s) ifTrue: { pc is %s - 1. }.",
              ccmp_str(inst, "True"), value_str(&inst->jmp));
    break;
  case JMP:
    emit_line("pc is %s - 1.",
              value_str(&inst->jmp));
    break;

  default:
    error("oops");
  }
}

void target_ctr(Module* module) {
  init_state_ctr(module->data);
  emit_line("");

  emit_line("var stdin is File fdopen: 0 mode: 'r'.");
  emit_line("var nextchar is {");
  inc_indent();
  emit_line("var c is stdin readBytes: 1.");
  emit_line("c length = 0 ifTrue: { ^0. }.");
  emit_line("^c byteAt: 0.");
  dec_indent();
  emit_line("}.");
  emit_line("var codes is Array <");
  inc_indent();

  int prev_pc = -1;
  for (Inst* inst = module->text; inst; inst = inst->next) {
    if (prev_pc != inst->pc) {
      if (prev_pc != -1) {
        emit_line("pc +=: 1.");
        emit_line("next is codes at: pc.");
        dec_indent();
        emit_line("}, at: 1) ;");
      }
      emit_line("(AST fromBlock: {");
      inc_indent();
      prev_pc = inst->pc;
    }
    ctr_emit_inst(inst);
  }

  dec_indent();
  emit_line("}, at: 1).");
  emit_line("var next is codes head.");
  emit_line("{ next evaluate. } forever.");
}
