// Original N_SOL src/bounty_hunter/Point.h; the (-1,-1) edge endpoint pair at e46c8.
#ifndef BOUNTY_HUNTER_POINT_H
#define BOUNTY_HUNTER_POINT_H
class CPoint {
public:
    // The Vector copy at e6024/e68c4 is only two words, without zeroing first; the user constructor semantics are kept.
    CPoint(int x = 0, int y = 0) : m_x(x), m_y(y) {}
    int m_x;
    int m_y;
};
#endif
