#include "ZonedModel.h"



std::string ZonedModel::GetZonedModelName()
{
	return zonedModelName;
}
void ZonedModel::SetZonedModelName(const std::string& zonedModel)
{
	zonedModelName = zonedModel;
}

void ZonedModel::SetZonedModelSuppressed(bool isSuppressed, bool type)
{
	auto modelHash = GAMEPLAY::GET_HASH_KEY((char*)zonedModelName.c_str());
	if (type)
		VEHICLE::SET_VEHICLE_MODEL_IS_SUPPRESSED(modelHash, isSuppressed);
	else
		PED::SET_PED_MODEL_IS_SUPPRESSED(modelHash, isSuppressed);
}