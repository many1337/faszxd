#pragma once

void WriteUsercmd(bf_write* buf, CUserCmd* in, CUserCmd* out)
{
	using WriteUserCmd_t = void(__fastcall*)(void*, CUserCmd*, CUserCmd*);
	static auto Fn = (WriteUserCmd_t)csgo->Utils.FindPatternIDA(GetModuleHandleA(g_Modules[fnva1(hs::client_dll.s().c_str())]().c_str()),
		hs::write_user_cmd.s().c_str());

	__asm
	{
		mov     ecx, buf
		mov     edx, in
		push    out
		call    Fn
		add     esp, 4
	}
}

bool __fastcall Hooked_WriteUsercmdDeltaToBuffer(void* ecx, void*, int slot, bf_write* buf, int from, int to, bool isnewcommand)
{
	static auto original_fn = g_pClientHook->GetOriginal <WriteUsercmdDeltaToBufferFn>(g_HookIndices[fnva1(hs::Hooked_WriteUsercmdDeltaToBuffer.s().c_str())]);

	if (!csgo->local
		|| !csgo->is_connected
		|| !csgo->is_local_alive
		|| csgo->game_rules->IsFreezeTime()
		|| csgo->local->HasGunGameImmunity()
		|| csgo->local->GetFlags() & FL_FROZEN)
		return original_fn(ecx, slot, buf, from, to, isnewcommand);

	if (!csgo->shift_amount)
		return original_fn(ecx, slot, buf, from, to, isnewcommand);


	if (from != -1)
		return true;

	auto final_from = -1;

	uintptr_t frame_ptr = 0;
	__asm mov frame_ptr, ebp;

	auto backup_commands = reinterpret_cast <int*> (frame_ptr + 0xFD8);
	auto new_commands = reinterpret_cast <int*> (frame_ptr + 0xFDC);

	auto newcmds = *new_commands;
	auto shift = csgo->shift_amount;

	csgo->shift_amount = 0;
	*backup_commands = 0;

	auto choked_modifier = newcmds + shift;

	if (choked_modifier > 62)
		choked_modifier = 62;

	*new_commands = choked_modifier;

	auto next_cmdnr = csgo->client_state->iChokedCommands + csgo->client_state->nLastOutgoingCommand + 1;
	auto final_to = next_cmdnr - newcmds + 1;

	if (final_to <= next_cmdnr)
	{
		while (original_fn(ecx, slot, buf, final_from, final_to, true))
		{
			final_from = final_to++;

			if (final_to > next_cmdnr)
				goto next_cmd;
		}

		return false;
	}
next_cmd:

	auto user_cmd = interfaces.input->GetUserCmd(final_from);

	if (!user_cmd)
		return true;

	CUserCmd to_cmd;
	CUserCmd from_cmd;

	from_cmd = *user_cmd;
	to_cmd = from_cmd;

	to_cmd.command_number++;
	to_cmd.tick_count += 200;

	if (newcmds > choked_modifier)
		return true;

	for (auto i = choked_modifier - newcmds + 1; i > 0; --i)
	{
		WriteUsercmd(buf, &to_cmd, &from_cmd);

		from_cmd = to_cmd;
		to_cmd.command_number++;
		to_cmd.tick_count++;
	}

	return true;
}