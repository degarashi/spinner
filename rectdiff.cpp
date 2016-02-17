#include "rectdiff.hpp"

namespace spn {
	namespace rect {
		int LoopValue(const int t, const int w) {
			if(t < 0)
				return (w-1)+((t+1)%w);
			return t % w;
		}
		int LoopValueD(const int t, const int w) {
			if(t < 0)
				return ((t+1)/w)-1;
			return t/w;
		}
		void IncrementalRect(const spn::Rect& base, const int dx, const int dy, const Rect_cb& cb) {
			// 動いて無ければ何もせず終了
			if(dx==0 && dy==0)
				return;
			// 前後の矩形が重ならない場合、単に移動後の矩形でコールバック
			if(std::abs(dx) >= base.width() ||
				std::abs(dy) >= base.height())
			{
				cb(base.move(dx, dy));
				return;
			}

			int x0=0, x1=0;
			int y0=0, y1=0;
			if(dx > 0) {
				x0 = base.x1;
				x1 = x0+dx;
			} else if(dx < 0) {
				x1 = base.x0;
				x0 = x1+dx;
			}
			if(dy > 0) {
				y0 = base.y1;
				y1 = y0+dy;
			} else if(dy < 0) {
				y1 = base.y0;
				y0 = y1+dy;
			}
			if(x0 != x1) {
				cb({x0,x1, base.y0+dy, base.y1+dy});
			}
			if(y0 != y1) {
				if(std::abs(dx) < base.width()) {
					if(dx > 0) {
						cb({base.x0+dx, base.x1, y0, y1});
					} else {
						cb({base.x0, base.x1+dx, y0, y1});
					}
				}
			}
		}
		void DivideRect(const spn::Size& size, const spn::Rect& request, const DRect_cb& cb) {
			const auto btm = [](auto x, auto w){
				return x/w*w;
			};
			struct Range {
				int g0, g1;
				int l0, l1;
			};
			auto fnRange = [&btm](Range (&res)[2], int x0, int x1, int w){
				const bool bX = LoopValueD(x0, w) != LoopValueD(x1, w);
				const auto lpx0 = LoopValue(x0,w),
							lpx1 = LoopValue(x1,w);
				if(bX) {
					const int mdl = btm(x1, w);
					res[0] = Range{x0,mdl, lpx0,w};
					res[1] = Range{mdl,x1, 0,lpx1};
					return (mdl==x1) ? 1 : 2;
				} else {
					res[0] = Range{x0,x1, lpx0,lpx1};
					return 1;
				}
			};
			const auto tw = size.width,
						th = size.height;
			Range xRange[2],
				yRange[2];
			const int nx = fnRange(xRange, request.x0, request.x1, tw),
					ny = fnRange(yRange, request.y0, request.y1, th);
			using Rect = spn::Rect;
			for(int i=0 ; i<ny ; i++) {
				auto& yr = yRange[i];
				for(int j=0 ; j<nx ; j++) {
					auto& xr = xRange[j];
					cb(
						Rect(
							xr.g0, xr.g1,
							yr.g0, yr.g1
						),
						Rect(
							xr.l0, xr.l1,
							yr.l0, yr.l1
						)
					);
				}
			}
		}
	}
}
