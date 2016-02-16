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
			return (t/w)+1;
		}
		void IncrementalRect(const spn::Rect& base, const int dx, const int dy, const Rect_cb& cb) {
			if(dx==0 && dy==0)
				return;

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
			// 更新領域のうちの左下(原点より)は常に更新
			const auto tw = size.width,
						th = size.height;
			const bool bX = LoopValueD(request.x0, tw) != LoopValueD(request.x1, tw),
					 bY = LoopValueD(request.y0, th) != LoopValueD(request.y1, th);
			const int x = LoopValue(request.x0, tw),
					y = LoopValue(request.y0, th);
			using Rect = spn::Rect;
			const Rect r(x, (bX ? tw : LoopValue(request.x1, tw)),
							y, (bY ? th : LoopValue(request.y1, th)));
			const auto btm = [](auto x, auto w){
				return x/w*w;
			};
			cb(
				Rect(
					request.x0,
					(bX ? btm(request.x1, tw) : request.x1),
					request.y0,
					(bY ? btm(request.y1, th) : request.y1)
				),
				r
			);
			// 更新領域のうちの右下
			if(bX) {
				cb(
					Rect(
						btm(request.x1, tw),
						request.x1,
						request.y0,
						(bY ? btm(request.y1, th) : request.y1)
					),
					Rect(
						0,
						LoopValue(request.x1, tw),
						r.y0,
						r.y1
					)
				);
			}
			// 更新領域のうちの左上
			if(bY) {
				cb(
					Rect(
						request.x0,
						btm(request.x1, tw),
						btm(request.y1, th),
						request.y1
					),
					Rect(
						r.x0,
						r.x1,
						0,
						LoopValue(request.y1, th)
					)
				);
			}
			// 更新領域のうちの右上
			if(bX && bY) {
				cb(
					Rect(
						btm(request.x1, tw),
						request.x1,
						btm(request.y1, th),
						request.y1
					),
					Rect(
						0,
						LoopValue(request.x1, tw),
						0,
						LoopValue(request.y1, th)
					)
				);
			}
		}
	}
}
