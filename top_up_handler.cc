#include "msg_dispatcher.h"
#include "top_up_rank.h"

int handle_top_up_req(const fighting::fighting_msgheader_t& header, 
		const void* pkg_ptr, uint32_t pkg_len, const fdsession_t* fdsess) {
	fighting::top_up_req req;
	DEBUG_LOG("hanlde top_up_req");
	if (false == req.ParseFromArray(pkg_ptr, pkg_len)) {
		ERROR_LOG("failed_parse_body_from_array handle_top_up_req");
		return -1;
	}
	GTopRank.Add(req.mutable_info());
	return 0;
}

int handle_top_up_rank_top_req(const fighting::fighting_msgheader_t& header,
		const void* pkg_ptr, uint32_t pkg_len, const fdsession_t* fdsess) {

	fighting::top_up_rank_top_req req;
	DEBUG_LOG("hanlde top_up_req_rank_top_req");
	if (false == req.ParseFromArray(pkg_ptr, pkg_len)) {
		ERROR_LOG("failed_parse_body_from_array handle_top_up_rank_top_req");
		return -1;
	}

	fighting::top_up_rank_top_rsp rsp;
	UserKey user;
	user.userid =( header.userid()<<32>>32);
	user.channel = header.userid()>>32;
	user.reg_tm  = header.reg_tm();
	GTopRank.GetTopN(&rsp, user, req.group(), 10);
	sMsgDispatcher.SendMsg(header, rsp, fdsess);
	DEBUG_LOG("send msg top_up_rank_top_rsp top_up_req_rank_top_req");
	return 0;
}

int handle_top_up_rank_three_req(const fighting::fighting_msgheader_t& header,
		const void* pkg_ptr, uint32_t pkg_len, const fdsession_t* fdsess) {
	fighting::top_up_rank_three_req	req;
	DEBUG_LOG("hanlde top_up_req_rank_three_req");
	if (false == req.ParseFromArray(pkg_ptr, pkg_len)) {
		ERROR_LOG("failed_parse_body_from_array handle_top_up_rank_top_req");
		return -1;
	}
	uint32_t group = req.group();
	fighting::top_up_rank_three_rsp rsp;
	UserKey user;
	user.userid = 0; user.channel = 0; user.reg_tm = 0;
	fighting::top_up_rank_top_rsp temp;
	GTopRank.GetTopN(&temp, user, group, 3);
	for (int i = 0; i < temp.infos_size(); ++i) {
		fighting::top_up_rank_three_rsp::userinfo *user = rsp.add_infos();
		user->set_zone_id(temp.infos(i).zone_id());
		user->set_name(temp.infos(i).name());
	}
	sMsgDispatcher.SendMsg(header, rsp, fdsess);
	return 0;
}


