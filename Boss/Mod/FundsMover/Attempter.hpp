#ifndef BOSS_MOD_FUNDSMOVER_ATTEMPTER_HPP
#define BOSS_MOD_FUNDSMOVER_ATTEMPTER_HPP

#include<cstdint>
#include<memory>

namespace Boss { namespace Mod { class Rpc; }}
namespace Ev { template<typename a> class Io; }
namespace Ln { class Amount; }
namespace Ln { class NodeId; }
namespace Ln { class Preimage; }
namespace Ln { class Scid; }
namespace S { class Bus; }

namespace Boss { namespace Mod { namespace FundsMover {

/** class Boss::Mod::FundsMover::Attempter
 *
 * @brief Makes an attempt to move a specific amount of funds
 * from one channel to another.
 *
 * @desc This object is dynamically created during runtime.
 */
class Attempter {
private:
	class Impl;
	std::shared_ptr<Impl> pimpl;

	Attempter() =default;

public:
	/* Return true if succeed, false if fail.  */
	static
	Ev::Io<bool>
	run( S::Bus& bus
	   , Boss::Mod::Rpc& rpc
	   , Ln::NodeId self
	   /* The preimage should have been pre-arranged to be claimed.  */
	   , Ln::Preimage preimage
	   , Ln::NodeId source
	   , Ln::NodeId destination
	   , Ln::Amount amount
	   , std::shared_ptr<Ln::Amount> fee_budget
	   /* Details of the channel from destination to us.  */
	   , Ln::Scid last_scid
	   , Ln::Amount base_fee
	   , std::uint32_t proportional_fee
	   , std::uint32_t cltv_delta
	   /* The channel from us to source.  */
	   , Ln::Scid first_scid
	   );
};

}}}

#endif /* !defined(BOSS_MOD_FUNDSMOVER_ATTEMPTER_HPP) */
