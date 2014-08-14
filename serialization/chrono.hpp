#pragma once
#include "serialization/traits.hpp"
#include <chrono>
#include <boost/serialization/split_free.hpp>

// boostに勝手にtypedefするのは不味そうなのでマクロで対処
//! 基準となる差分形式
#define Duration_t	std::chrono::nanoseconds
#define Rep_t		Duration_t::rep
namespace boost {
	namespace serialization {
		template <class Archive, class C, class D>
		void serialize(Archive& ar, std::chrono::time_point<C,D>& tp, const unsigned int ver) {
			split_free(ar, tp, ver);
		}
		template <class Archive, class C, class D>
		void load(Archive& ar, std::chrono::time_point<C,D>& tp, const unsigned int) {
			Rep_t rep;
			std::time_t epoch;
			ar & BOOST_SERIALIZATION_NVP(rep) & BOOST_SERIALIZATION_NVP(epoch);

			Duration_t dur(rep);
			// 現在使用中ClockのEpochタイムとの差分を考慮 (別PCでの読み込みなど)
			auto tp2 = std::chrono::time_point_cast<Duration_t>(C::from_time_t(epoch));
			tp = std::chrono::time_point<C,D>(dur - tp2.time_since_epoch());
		}
		template <class Archive, class C, class D>
		void save(Archive& ar, const std::chrono::time_point<C,D>& tp, const unsigned int) {
			// Epochをtime_tに変換したものと、Epochからの差分を保存
			// time_tはPOSIX timeのEpochなので機種間で共通
			auto rep = std::chrono::duration_cast<Duration_t>(tp.time_since_epoch()).count();
			std::time_t epoch = C::to_time_t(std::chrono::time_point<C,D>(Duration_t(0)));
			ar & BOOST_SERIALIZATION_NVP(rep) & BOOST_SERIALIZATION_NVP(epoch);
		}

		template <class Archive, class R, class P>
		void serialize(Archive& ar, std::chrono::duration<R,P>& dur, const unsigned int ver) {
			split_free(ar, dur, ver);
		}
		template <class Archive, class R, class P>
		void load(Archive& ar, std::chrono::duration<R,P>& dur, const unsigned int) {
			using Src = std::chrono::duration<R,P>;
			Rep_t rep;
			ar & BOOST_SERIALIZATION_NVP(rep);
			dur = std::chrono::duration_cast<Src>(Duration_t(rep));
		}
		template <class Archive, class R, class P>
		void save(Archive& ar, const std::chrono::duration<R,P>& dur, const unsigned int) {
			// 全てnanosecondsに変換して保存
			auto dur2 = std::chrono::duration_cast<Duration_t>(dur);
			auto rep = dur2.count();
			ar & BOOST_SERIALIZATION_NVP(rep);
		}
	}
}
#undef Duration_t
#undef Rep_t
BOOST_CLASS_IMPLEMENTATION_TEMPLATE((class)(class), std::chrono::time_point, object_serializable)
BOOST_CLASS_TRACKING_TEMPLATE((class)(class), std::chrono::time_point, track_never)
