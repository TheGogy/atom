#ifndef TUNABLES_H
#define TUNABLES_H

namespace Atom {

namespace Tunables {


constexpr int ASPIRATION_WINDOW_SIZE    = 10;
constexpr int ASPIRATION_WINDOW_DIVISOR = 120;

constexpr int DELTA_INCREMENT_DIV = 3;

constexpr int UPDATE_NODES = 1000000;

constexpr int IIR_REDUCTION = 3;

constexpr int RFP_DEPTH = 4;
constexpr int RFP_DEPTH_MULTIPLIER = 100;

constexpr int RAZORING_DEPTH = 2;
constexpr int RAZORING_DEPTH_MULTIPLIER = 400;

constexpr int FUTILITY_PRUNING_DEPTH = 13;
constexpr int FUTILITY_MULT_BASE = 122;
constexpr int FUTILITY_TTCUT_IMPACT = 37;
constexpr int FUTILITY_IMPROVEMENT_SCALE = 2;
constexpr int FUTILITY_WORSENING_SCALE = 3;
constexpr int FUTILITY_STAT_SCALE = 260;

constexpr int NMR_EVAL_SCALE = 202;
constexpr int NMR_EVAL_MAX_DIFF = 6;
constexpr int NMR_DEPTH_SCALE = 3;
constexpr int NMR_MIN_REDUCTION = 5;

constexpr int NMP_VERIFICATION_MIN_DEPTH = 16;
constexpr int NMP_DEPTH_SCALE = 3;
constexpr int NMP_DEPTH_DIVISOR = 4;

constexpr int MOVEPICK_CAPTURE_MULTIPLIER = 7;
constexpr int MOVEPICK_KILLER_SCORE = 65536;
constexpr int MOVEPICK_CHECK_SCORE  = 16384;

constexpr int MOVEPICK_ESCAPE_QUEEN = 51700;
constexpr int MOVEPICK_ESCAPE_ROOK  = 25600;
constexpr int MOVEPICK_ESCAPE_MINOR = 14450;

constexpr int MOVEPICKER_ENPRISE_QUEEN = 49000;
constexpr int MOVEPICKER_ENPRISE_ROOK  = 24335;
constexpr int MOVEPICKER_ENPRISE_MINOR = 14900;


} // namspace Tunables

} // namespace Atom

#endif //TUNABLES_H
