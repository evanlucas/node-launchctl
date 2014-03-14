#
# ellog.bash - an npm-like logging facility for shell scripts
#

ELLOG_VERSION="0.0.1"
ELLOG_HEADING=${ELLOG_HEADING-ellog}
ELLOG_LEVEL=${ELLOG_LEVEL-40}

# levels
ELLOG_LEVEL_SILLY="10"
ELLOG_LEVEL_VERB="20"
ELLOG_LEVEL_HTTP="30"
ELLOG_LEVEL_INFO="40"
ELLOG_LEVEL_WARN="50"
ELLOG_LEVEL_ERROR="60"
ELLOG_LEVEL_SILENT="70"

WHITE_FG=$(tput setaf 15)
MAGENTA_FG=$(tput setaf 171)
YELLOW_FG=$(tput setaf 155)
GREEN_FG=$(tput setaf 41)
YELLOW_BG=$(tput setab 155)
RED_FG=$(tput setaf 124)
BLUE_FG=$(tput setaf 12)
GREEN_FG2=$(tput setaf 34)
BLACK_FG=$(tput setaf 000)
REVERSE=$(tput rev)

RESET=$(tput sgr0)
BOLD=$(tput bold)

ellog_set_heading() {
  ELLOG_HEADING="$1"
}

ellog_set_level() {
  case $1 in
    silly) ELLOG_LEVEL="10" ;;
    verbose) ELLOG_LEVEL="20" ;;
    http) ELLOG_LEVEL="30" ;;
    info) ELLOG_LEVEL="40" ;;
    warn) ELLOG_LEVEL="50" ;;
    error) ELLOG_LEVEL="60" ;;
    silent) ELLOG_LEVEL="70" ;;
    *) echo "${RED_FG}INVALID LOG LEVEL${RESET}" ;;
  esac
}

ellog_silly() {
  if [[ "$ELLOG_LEVEL" == ${ELLOG_LEVEL_SILLY} ]]; then
    echo -n "${WHITE_FG}${ELLOG_HEADING}${RESET}" "${REVERSE}sill${RESET}" "${MAGENTA_FG}$1${RESET} "
    shift
    echo "$@"
  fi
}

ellog_verbose() {
  if [[ "$ELLOG_LEVEL" -le ${ELLOG_LEVEL_VERB} ]]; then
    echo -n "${WHITE_FG}${ELLOG_HEADING}${RESET}" "${BLUE_FG}verb${RESET}" "${MAGENTA_FG}$1${RESET} "
    shift
    echo "$@"
  fi
}

ellog_http() {
  if [[ "$ELLOG_LEVEL" -le ${ELLOG_LEVEL_HTTP} ]]; then
    echo -n "${WHITE_FG}${ELLOG_HEADING}${RESET}" "${GREEN_FG2}http${RESET}" "${MAGENTA_FG}$1${RESET} "
    shift
    echo "$@"
  fi
}

ellog_info() {
  if [[ "$ELLOG_LEVEL" -le ${ELLOG_LEVEL_INFO} ]]; then
    echo -n "${WHITE_FG}${ELLOG_HEADING}${RESET}" "${GREEN_FG}info${RESET}" "${MAGENTA_FG}$1${RESET} "
    shift
    echo "$@"
  fi
}

ellog_warn() {
  if [[ "$ELLOG_LEVEL" -le ${ELLOG_LEVEL_WARN} ]]; then
    echo -n "${WHITE_FG}${ELLOG_HEADING}${RESET}" "${BLACK_FG}${YELLOW_BG}WARN${RESET}" "${MAGENTA_FG}$1${RESET} "
    shift
    echo "$@"
  fi
}

ellog_error() {
  if [[ "$ELLOG_LEVEL" -le ${ELLOG_LEVEL_ERROR} ]]; then
    echo -n "${WHITE_FG}${ELLOG_HEADING}${RESET}" "${RED_FG}ERR!${RESET}" "${MAGENTA_FG}$1${RESET} "
    shift
    echo "$@"
  fi
}
