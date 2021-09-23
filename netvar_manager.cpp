#include "netvar_manager.h"
#include "interfaces.h"



void CNetVarManager::Initialize()
{
	m_tables.clear();

	ClientClass* clientClass = interfaces.client->GetAllClasses();
	if (!clientClass)
		return;

	while (clientClass)
	{
		RecvTable* recvTable = clientClass->m_pRecvTable;
		m_tables.push_back(recvTable);

		clientClass = clientClass->m_pNext;
	}
}

int CNetVarManager::GetOffset(const char* tableName, const char* propName)
{
	int offset = Get_Prop(tableName, propName);
	if (!offset)
	{
		return 0;
	}
	return offset;
}

bool CNetVarManager::HookProp(const char* tableName, const char* propName, RecvVarProxyFn fun)
{
	RecvProp* recvProp = 0;
	Get_Prop(tableName, propName, &recvProp);
	if (!recvProp)
		return false;

	recvProp->m_ProxyFn = fun;

	return true;
}

int CNetVarManager::Get_Prop(const char* tableName, const char* propName, RecvProp** prop)
{
	RecvTable* recvTable = GetTable(tableName);
	if (!recvTable)
		return 0;

	int offset = Get_Prop(recvTable, propName, prop);
	if (!offset)
		return 0;

	return offset;
}

int CNetVarManager::Get_Prop(RecvTable* recvTable, const char* propName, RecvProp** prop)
{
	int extraOffset = 0;
	for (int i = 0; i < recvTable->m_nProps; ++i)
	{
		RecvProp* recvProp = &recvTable->m_pProps[i];
		RecvTable* child = recvProp->m_pDataTable;

		if (child && (child->m_nProps > 0))
		{
			int tmp = Get_Prop(child, propName, prop);
			if (tmp)
				extraOffset += (recvProp->m_Offset + tmp);
		}

		if (_stricmp(recvProp->m_pVarName, propName))
			continue;

		if (prop)
			*prop = recvProp;

		return (recvProp->m_Offset + extraOffset);
	}

	return extraOffset;
}

RecvTable* CNetVarManager::GetTable(const char* tableName)
{
	if (m_tables.empty())
		return 0;

	for(RecvTable* table : m_tables)
	{
		if (!table)
			continue;

		if (_stricmp(table->m_pNetTableName, tableName) == 0)
			return table;
	}

	return 0;
}

void CNetVarManager::DumpTable(RecvTable* table, int depth)
{
	std::string pre(str(""));
	for (int i = 0; i < depth; i++)
		pre.append(str("\t"));

	for (int i = 0; i < table->m_nProps; i++)
	{
		RecvProp* prop = &table->m_pProps[i];
		if (!prop)
			continue;

		std::string varName(prop->m_pVarName);

		if (varName.find(str("baseclass")) == 0 || varName.find(str("0")) == 0 || varName.find(str("1")) == 0 || varName.find(str("2")) == 0)
			continue;
		if (prop->m_pDataTable)
			DumpTable(prop->m_pDataTable, depth + 1);
	}
}
