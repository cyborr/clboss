#include"Boss/Mod/ChannelCandidateInvestigator/Gumshoe.hpp"
#include"Boss/Mod/ChannelCandidateInvestigator/Manager.hpp"
#include"Boss/Mod/ChannelCandidateInvestigator/Secretary.hpp"
#include"Boss/Msg/Init.hpp"
#include"Boss/Msg/ListpeersResult.hpp"
#include"Boss/Msg/ProposeChannelCandidates.hpp"
#include"Boss/Msg/SolicitChannelCandidates.hpp"
#include"Boss/Msg/TimerRandomHourly.hpp"
#include"Boss/log.hpp"
#include"Boss/random_engine.hpp"
#include"Ev/map.hpp"
#include"Ln/NodeId.hpp"
#include"S/Bus.hpp"
#include"Sqlite3.hpp"
#include<random>

namespace {

/* If we are below this number of non-negative candidates,
 * solicit more.  */
auto const min_good_candidates = std::size_t(3);

/* Nodes below this score should just be dropped.  */
auto const min_score = std::int64_t(-3);

/* Number of candidates to investigate in parallel.  */
auto const max_investigation = std::size_t(8);

}

namespace Boss { namespace Mod { namespace ChannelCandidateInvestigator {

void Manager::start() {
	/* Give the gumshoe the report function.  */
	gumshoe.set_report_func([this](Ln::NodeId n, bool success) {
		return db.transact().then([this, n, success](Sqlite3::Tx tx) {
			secretary.update_score( tx, n
					      , success ? +1 : -1
					      , min_score
					      );
			tx.commit();

			return Ev::lift();
		});
	});

	bus.subscribe<Msg::Init>([this](Msg::Init const& init) {
		db = init.db;

		/* Initialize the database.  */
		return db.transact().then([this](Sqlite3::Tx tx) {
			secretary.initialize(tx);
			/* Get number of good candidates.  */
			auto good_candidates =
				secretary.get_nonnegative_candidates_count(tx);
			tx.commit();

			if (good_candidates < min_good_candidates)
				return solicit_candidates(good_candidates);

			return Ev::lift();
		});
	});
	/* When a new candidate is proposed, add it.  */
	bus.subscribe<Msg::ProposeChannelCandidates
		     >([this](Msg::ProposeChannelCandidates const& c) {
		return db.transact().then([this, c](Sqlite3::Tx tx) {
			secretary.add_candidate(tx, c);
			tx.commit();

			return Ev::lift();
		});
	});

	bus.subscribe<Msg::TimerRandomHourly
		     >([this](Msg::TimerRandomHourly const& _) {
		/* Construct shared variables.  */
		/* Number of good candidates.  */
		auto good_candidates = std::make_shared<std::size_t>();
		/* What cases to give to the gumshoe.  */
		auto cases = std::make_shared<std::vector<Ln::NodeId>>();
		/* Human-readable text.  */
		auto report = std::make_shared<std::string>();

		return db.transact().then([=](Sqlite3::Tx tx) {
			/* Determine number of good candidates.  */
			*good_candidates =
				secretary.get_nonnegative_candidates_count(tx);
			/* Get some nodes for investigation.  */
			*cases = secretary.get_for_investigation(
				tx, max_investigation
			);
			/* Get a report.  */
			*report = secretary.report(tx);
			/* Finish up.  */
			tx.commit();

			/* Print the report.  */
			return Boss::log( bus, Info
					, "ChannelCandidateInvestigator: %s"
					, report->c_str()
					);
		}).then([=]() {
			/* If too few candidates, get more.  */
			if (*good_candidates < min_good_candidates)
				return solicit_candidates(*good_candidates);
			/* Even if we have sufficient, have a chance to
			 * get more.
			 */
			auto dist = std::uniform_int_distribution<std::size_t>(
				0, 2 * (*good_candidates)
			);
			if (dist(Boss::random_engine) == 0)
				return solicit_candidates(*good_candidates);

			return Ev::lift();
		}).then([=]() {
			/* Hand over possible cases to the gumshoe.  */
			auto to_gumshoe = [this](Ln::NodeId n) {
				return gumshoe.investigate(n).then([]() {
					return Ev::lift<int>(0);
				});
			};
			return Ev::map(to_gumshoe, std::move(*cases));
		}).then([](std::vector<int>) {

			return Ev::lift();
		});
	});

	/* Remove candidates we already have a channel with.  */
	bus.subscribe<Msg::ListpeersResult
		     >([this](Msg::ListpeersResult const& l) {
		auto to_remove = std::make_shared<std::vector<Ln::NodeId>>();
		for (auto i = std::size_t(0); i < l.peers.size(); ++i) {
			auto peer = l.peers[i];
			if (!peer.is_object() || !peer.has("id"))
				continue;
			if (!peer.has("channels"))
				continue;
			auto id = peer["id"];
			if (!id.is_string())
				continue;
			auto node = Ln::NodeId(std::string(id));
			auto chans = peer["channels"];
			if (!chans.is_array())
				continue;

			for (auto j = std::size_t(0); j < chans.size(); ++j) {
				auto chan = chans[i];
				if (!chan.is_object() || !chan.has("state"))
					continue;
				auto state = chan["state"];
				if (!state.is_string())
					continue;
				auto state_s = std::string(state);
				if (state_s.size() < 8)
					continue;
				auto prefix = std::string( state_s.begin()
							 , state_s.begin() + 8
							 );
				if ( prefix != "CHANNELD"
				  && prefix != "OPENINGD"
				   )
					continue;
				/* We have a live channel with them!  */
				to_remove->emplace_back(std::move(node));
				break;
			}
		}

		/* Now have the secretary update our records.  */
		return db.transact().then([this, to_remove](Sqlite3::Tx tx) {
			for (auto const& n : *to_remove)
				secretary.remove_candidate(tx, n);
			tx.commit();
			return Ev::lift();
		});
	});
}
Ev::Io<void> Manager::solicit_candidates(std::size_t good_candidates) {
	return Boss::log( bus, Debug
			, "ChannelCandidateInvestigator: "
			  "Have %zu channel candidates, "
			  "soliciting more."
			, good_candidates
			).then([this]() {
		return bus.raise(
			Msg::SolicitChannelCandidates{}
		);
	});
}

}}}
