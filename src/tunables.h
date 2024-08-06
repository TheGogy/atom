#ifndef TUNABLES_H
#define TUNABLES_H

namespace Atom {

namespace Tunables {


constexpr int ASPIRATION_WINDOW_SIZE    = 5;
constexpr int ASPIRATION_WINDOW_DIVISOR = 13424;

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
constexpr int MOVEPICK_KILLER_SCORE = 1 << 16;
constexpr int MOVEPICK_CHECK_SCORE  = 16384;

constexpr int MOVEPICK_ESCAPE_QUEEN = 51700;
constexpr int MOVEPICK_ESCAPE_ROOK  = 25600;
constexpr int MOVEPICK_ESCAPE_MINOR = 14450;

constexpr int MOVEPICKER_ENPRISE_QUEEN = 49000;
constexpr int MOVEPICKER_ENPRISE_ROOK  = 24335;
constexpr int MOVEPICKER_ENPRISE_MINOR = 14900;

constexpr int MOVEPICKER_LOSING_CAP_THRESHOLD = 18;
constexpr int MOVEPICKER_QUIET_THRESHOLD = -3560;
constexpr int MOVEPICKER_GOOD_QUIET_THRESHOLD = -7998;

constexpr int CUTNODE_MIN_DEPTH = 7;

constexpr int SEE_PRUNING_MAX_DEPTH = 10;
constexpr int SEE_PRUNING_CAP_SCORE = 180;
constexpr int SEE_PRUNING_CHK_SCORE = 70;

constexpr double REDUCTION_AMOUNT = 18.62;
constexpr int REDUCTION_BASE = 1274;
constexpr int REDUCTION_DELTA_SCALE = 746;
constexpr int REDUCTION_NORMALISER = 1024;
constexpr int REDUCTION_SCALE_THRESHOLD = 1293;

constexpr int SCORE_IMPROVEMENT_DEPTH_MIN = 2;
constexpr int SCORE_IMPROVEMENT_DEPTH_MAX = 14;

constexpr int PREVIOUS_POS_TTPV_MIN_DEPTH = 3;

constexpr int SEE_PRUNING_QSEARCH_SKIP_THRESHOLD = -83;

constexpr int NNUE_SMALL_NET_THRESHOLD = 962;
constexpr int NNUE_PSQT_WEIGHT = 125;
constexpr int NNUE_POSITIONAL_WEIGHT = 131;

constexpr int NNUE_BASE_EVAL = 73921;

constexpr int EVALUATION_NORMALIZER_SMALLNET = 68104;
constexpr int EVALUATION_NORMALIZER_BIGNET   = 74715;

constexpr int REDUCTION_HIGH_THRESHOLD = 5;

} // namspace Tunables

} // namespace Atom

#endif //TUNABLES_H
