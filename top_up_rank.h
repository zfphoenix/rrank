#ifndef TOP_UP_RANK_H_
#define TOP_UP_RANK_H_
extern "C" {
#include <stdint.h>
}
#include <map>
#include <vector>
#include <cstdlib>

#include "object_leveldb.h"
#include "base/Singleton.hpp"
//not support server-combination
#include "proto/fighting_rank.pb.h" 
struct UserKey;
struct UserValue;
class  TopRank;

namespace fighting {
class top_up_rank_top_rsp;
class TopUpUserInfo;
}
std::string format_key(uint32_t userid, uint32_t channel, uint32_t reg_tm);
int parse_key(const std::string &key, uint32_t &userid, uint32_t &channel, uint32_t &reg_tm);

std::string format_value(uint32_t zone_id, uint32_t top_up, uint32_t group, std::string name);
int parse_value(const std::string &value, uint32_t &zone_id, uint32_t &top_up, uint32_t &group, std::string &name);

struct UserKey {
	uint32_t userid;
	uint32_t channel;
	uint32_t reg_tm;

	bool operator==(const UserKey &right) const {
		return userid == right.userid && channel == right.channel && reg_tm == right.reg_tm;
	}
	std::string Format() {
		return format_key(userid, channel, reg_tm);
	}
	bool operator<(const UserKey &right) const {
		if (userid < right.userid)
			return true;
		if (userid > right.userid)
			return false;
		if (reg_tm < right.reg_tm)
			return true;
		if (reg_tm > right.reg_tm)
			return false;
		return channel < right.channel;
	}
};

struct UserValue {
	uint32_t zone_id;
	uint32_t top_up;
	uint32_t group;
	std::string name;
	std::string Format() {
		return format_value(zone_id, top_up, group, name);
	}
};

//while 'group' changed, update with new top up value;
//
class TopRank : public Singleton<TopRank> {
public:
	TopRank():odb_(NULL) {
		odb_ = new ObjectDB<>("top_up_leveldb"); //XXX
		init();
	}
	~TopRank() {
		if (odb_) {
			delete odb_;
			odb_ = NULL;
		}
	}

	void init();

	void GetTopN(fighting::top_up_rank_top_rsp *rsp, const UserKey &user,
			uint32_t n, uint32_t group);

	void Add(const fighting::TopUpUserInfo *info);

	uint32_t GetUserRank(const UserKey &key);

private:
	ObjectDB<> *odb_;
	typedef std::map< UserKey, UserValue > UserInfoMap;
	typedef UserInfoMap::iterator		   UserInfoItr;
	//		std::map < group, std::multimap < top_up, key > >
	// this map used to get top `n` rank
	typedef std::map< uint32_t, std::multimap < uint32_t, UserKey > > GroupRankMap;
	typedef GroupRankMap::iterator		   GroupRankItr;
	typedef std::multimap< uint32_t, UserKey >::iterator	MultiUserItr;
	UserInfoMap  user_info_map_;
	GroupRankMap group_rank_map_;
};

#define GTopRank TopRank::get_singleton()

#endif  //TOP_UP_RANK_H_
