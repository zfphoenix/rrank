#include "top_up_rank.h"
#include "proto/fighting_rank.pb.h"
#include <libtaomee/log.h>

std::string format_key(uint32_t userid, uint32_t channel, uint32_t reg_tm) {
	char buff[64];
	std::string str;
	int len = sprintf(buff, "%u:%u:%u", userid, channel, reg_tm);
	str.append(buff, len);
	return str;
}

int parse_key(const std::string &key, uint32_t &userid, uint32_t &channel, uint32_t &reg_tm) {
	return sscanf(key.c_str(), "%u:%u:%u", &userid, &channel, &reg_tm);
}

std::string format_value(uint32_t zone_id, uint32_t top_up, uint32_t group, std::string name) {
	char buff[256];
	std::string str;
	int len = sprintf(buff, "%u:%u:%u:%s",zone_id, top_up, group, name.c_str());
	str.append(buff, len);
	return str;
}

int parse_value(const std::string &value, uint32_t &zone_id, uint32_t &top_up, uint32_t &group, std::string &name) {
	char buff[64];
	int ret = sscanf(value.c_str(), "%u:%u:%u:%s", &zone_id, &top_up, &group, buff);
	name.clear();
	name.append(buff, strlen(buff));
	return ret;
}

//implemention of TopRank
void TopRank::init() {
	DEBUG_LOG("initialize top_up_level_db to cache");
	leveldb::DB* db = NULL;
	uint32_t count = 0;
	if (odb_ && (db = odb_->GetDb())) {
		leveldb::Iterator* itr = db->NewIterator(leveldb::ReadOptions());
		for (itr->SeekToFirst(); itr->Valid(); itr->Next()) {
			UserKey key;
			UserValue value;
			parse_key(itr->key().ToString(), key.userid, key.channel, key.reg_tm);
			parse_value(itr->value().ToString(), value.zone_id, value.top_up, 
					value.group, value.name);
			user_info_map_[key] = value;
			group_rank_map_[value.group].insert(std::make_pair<uint32_t, UserKey>(value.top_up ,key));
			++count;
		}
		delete itr;
		DEBUG_LOG("%s initial with %u users", __func__, count);
	} else {
		//TODO 
		ERROR_LOG("%s cannot open top_up_level_db", __func__);
		assert(0);
	}
}

void TopRank::GetTopN(fighting::top_up_rank_top_rsp *rsp, const UserKey &user, 
		uint32_t group, uint32_t n) {
	std::vector<UserKey> user_key_vect;
	std::multimap<uint32_t, UserKey>::reverse_iterator itr = group_rank_map_[group].rbegin();
	for (; n > 0 && itr != group_rank_map_[group].rend(); ++itr) {
		--n;
		user_key_vect.push_back(itr->second);
	}
	uint32_t rank = 1;
	bool     is_contain = false;
	for (size_t i = 0; i < user_key_vect.size(); ++i) {
		if (user_info_map_.find(user_key_vect[i]) != user_info_map_.end()) {
			//fill rsp package
			if (!is_contain && user_key_vect[i] == user) {
				rsp->set_rank(rank);
				rsp->set_total_top_up(user_info_map_[user].top_up);
				is_contain = true;
			}
			fighting::TopUpUserInfo *info = rsp->add_infos();
			info->set_userid(user_key_vect[i].userid);
			info->set_channel(user_key_vect[i].channel);
			info->set_reg_tm(user_key_vect[i].reg_tm);
			info->set_zone_id(user_info_map_[user_key_vect[i]].zone_id);
			info->set_name(user_info_map_[user_key_vect[i]].name);
			info->set_top_up(user_info_map_[user_key_vect[i]].top_up);
			info->set_group(user_info_map_[user_key_vect[i]].group);
			info->set_rank(rank);
			DEBUG_LOG("GetTopN: %d %d %d %d %s %d %d %d", user_key_vect[i].userid, user_key_vect[i].channel, 
					user_key_vect[i].reg_tm, user_info_map_[user_key_vect[i]].zone_id, user_info_map_[user_key_vect[i]].name.c_str(),
					user_info_map_[user_key_vect[i]].top_up, user_info_map_[user_key_vect[i]].group, rank);
			++rank;
		}
	}
	if (user_info_map_.find(user) != user_info_map_.end()) {
		if (!is_contain) {
			rsp->set_rank(GetUserRank(user));
			rsp->set_total_top_up(user_info_map_[user].top_up);
		}
	} else {
		rsp->set_rank(0);
		rsp->set_total_top_up(0);
	}
}

void TopRank::Add(const fighting::TopUpUserInfo *info) {
	UserKey key;
	UserValue value;
	key.userid	= info->userid();
	key.reg_tm  = info->reg_tm();
	key.channel	= info->channel();
	value.zone_id = info->zone_id();
	value.top_up  = info->top_up();
	value.group   = info->group();
	value.name    = info->name();
	UserInfoItr itr = user_info_map_.find(key);

	if (itr != user_info_map_.end()) {
#if 0 //暂不支持合服,合服需要清除数据重启
		if (itr->second.zone_id != value.zone_id)
			user_info_map_[key] = value;
			
		else if (itr->second.group != value.group) {
			//TODO delete from old group and refill new group
		} else {
			value.top_up += itr->second.top_up;
			//TODO update Group_Rank;
		}
#endif
		value.top_up += itr->second.top_up;
		std::pair <std::multimap<uint32_t, UserKey>::iterator, std::multimap<uint32_t, UserKey>::iterator> equal_itr;
		equal_itr = group_rank_map_[value.group].equal_range(itr->second.top_up);
		MultiUserItr mitr = equal_itr.first;
		for (; mitr != equal_itr.second; ) {
			if (key == mitr->second) {
				group_rank_map_[value.group].erase(mitr++);
				break;
			} else {
				++mitr;
			}
		}
	}
	user_info_map_[key] = value;
	group_rank_map_[value.group].insert(std::make_pair<uint32_t, UserKey>(value.top_up, key));
	odb_->write(key.Format(), value.Format());
	DEBUG_LOG("TopRank::Add key %s value %s", key.Format().c_str(), value.Format().c_str());
}

// 0 代表未上榜
uint32_t TopRank::GetUserRank(const UserKey &key) {
	UserInfoItr itr = user_info_map_.find(key);
	if (itr != user_info_map_.end()) {
		uint32_t total = group_rank_map_[itr->second.group].size();
		uint32_t rank = 0;
		MultiUserItr user_itr = group_rank_map_[itr->second.group].find(itr->second.top_up);
		if (user_itr != group_rank_map_[itr->second.group].end()) {
			//XXX 判断中间值 减少循环次数
			MultiUserItr begin_itr = group_rank_map_[itr->second.group].begin();
			rank = 1;
			for (; begin_itr != user_itr; ++begin_itr) {
				++rank;
			}
		}
		if (rank > total) return 0;
		return (total - rank + 1);
	} else {
		return 0;
	}
	return 0;
}


