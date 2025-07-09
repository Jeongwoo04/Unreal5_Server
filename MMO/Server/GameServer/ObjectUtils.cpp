#include "pch.h"
#include "ObjectUtils.h"
#include "Player.h"
#include "GameSession.h"

atomic<int64> ObjectUtils::s_idGenerator = 1;

PlayerRef ObjectUtils::CreatePlayer(GameSessionRef session)
{
	// ID »ý¼º
	const int64 newId = s_idGenerator.fetch_add(1);
	
	PlayerRef player = make_shared<Player>();
	//player->_objectInfo.set_object_id(newId);
	//player->_posInfo.set_object_id(newId);

	player->_session = session;
	session->_player = player;

	return player;
}
