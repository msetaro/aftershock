#include "../../engine/botlib/be_ai_weight.cpp"
#include <assert.h>
int main() {
	fuzzyseperator_t nodes[3] = {
		{ 0, 10, WT_BALANCE, 7.5f, -1, 12, nullptr, nullptr },
		{ 1, 100, WT_BALANCE, 35.25f, 10, 50, nullptr, nullptr },
		{ 0, 30, WT_BALANCE, 99.5f, 75, 120, nullptr, nullptr }
	};
	nodes[0].child = &nodes[1];
	nodes[0].next = &nodes[2];
	char name[] = "range_weapon";
	weightconfig_t original{};
	original.numweights = 1;
	original.weights[0] = { name, &nodes[0] };
	strcpy( original.filename, "bots/checkpoint_w.c" );
	static unsigned char bytes[131072];
	stateWriter_t writer{ bytes, sizeof( bytes ) };
	assert(Bot_WriteWeightState(&writer,129,&original));
	stateReader_t reader;
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	fuzzyseperator_t copied[3];
	memcpy( copied, nodes, sizeof( copied ) );
	copied[0].child = &copied[1];
	copied[0].next = &copied[2];
	for ( auto &node : copied )
		node.weight = node.minweight = node.maxweight = 0;
	auto restored = original;
	restored.weights[0].firstseperator = copied;
	assert(Bot_ReadWeightState(reader,129,&restored,false) && copied[0].weight==0);
	assert(Bot_ReadWeightState(reader,129,&restored,true));
	for ( int i = 0; i < 3; ++i )
		assert(copied[i].weight==nodes[i].weight && copied[i].minweight==nodes[i].minweight && copied[i].maxweight==nodes[i].maxweight);
	for ( int i = 0; i < 64; ++i ) {
		int inventory[2] = { i, 50 + i };
		const float first = FuzzyWeight( inventory, &original, 0 ), second = FuzzyWeight( inventory, &restored, 0 );
		assert(!memcmp(&first,&second,sizeof(first)));
	}
	copied[0].value++;
	assert(!Bot_ReadWeightState(reader,129,&restored,true) && copied[0].value==11);
	copied[0].value--;
	weightIdentitySave_t identity;
	uint32_t version;
	assert(State_Find(reader,weightIdentitySchema,129,&identity,&version));
	writer = { bytes, sizeof( bytes ) };
	assert(State_Append(&writer,weightIdentitySchema,129,&identity));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	copied[2].weight = 777;
	assert(!Bot_ReadWeightState(reader,129,&restored,true) && copied[2].weight==777);
	nodes[2].next = &nodes[0];
	writer = { bytes, sizeof( bytes ) };
	assert(!Bot_WriteWeightState(&writer,129,&original) && !State_Finish(&writer));
	nodes[2].next = nullptr;
	nodes[1].weight = HUGE_VALF;
	writer = { bytes, sizeof( bytes ) };
	assert(!Bot_WriteWeightState(&writer,129,&original) && !State_Finish(&writer));
	writer = { bytes, sizeof( bytes ) };
	assert(Bot_WriteWeightState(&writer,129,nullptr));
	assert(State_Open(bytes,State_Finish(&writer),&reader));
	assert(Bot_ReadWeightState(reader,129,nullptr,true));
	assert(!Bot_ReadWeightState(reader,129,&restored,true));
	// Bound checks reject an oversized tree without truncating its live values.
	static fuzzyseperator_t oversized[MAX_SAVED_WEIGHT_NODES + 1];
	for ( uint32_t i = 0; i < MAX_SAVED_WEIGHT_NODES; ++i )
		oversized[i].next = &oversized[i + 1];
	original.weights[0].firstseperator = oversized;
	writer = { bytes, sizeof( bytes ) };
	assert(!Bot_WriteWeightState(&writer,129,&original) && !State_Finish(&writer));
	puts( "PASS: mutable bot weights restore against matching relocated trees with identical evaluation" );
}
