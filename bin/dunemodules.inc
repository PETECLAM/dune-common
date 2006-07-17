
###############################################
###
### Configuration
###

# name of the "control" files
CONTROL="dune.module"

###############################################


#
# read paramters from a $CONTROL file
#
# paramters:
# $1 file to read
#
parse_control() {
  if test -f "$1"; then
    export module=$(echo $(grep Module: "$1" | cut -d ':' -f2))
    local deps=$(echo $(grep Depends: "$1" | cut -d ':' -f2))
    local sugs=$(echo $(grep Suggests: "$1" | cut -d ':' -f2))
    local path=$(dirname "$1")
    if test "x$module" != "x"; then
      if check_modname "$module"; then
        export HAVE_${module}=yes
        export PATH_${module}="$path"
        export DEPS_${module}="$deps"
        export SUGS_${module}="$sugs"
      else
        echo "ERROR: $CONTROL files $1 contains an invalid Module entry"
        exit 1
      fi
    else
      echo "ERROR: $CONTROL files $1 does not contain a Module entry"
      exit 1
    fi
  else
    echo "ERROR: could not read file $1"
    exit 1
  fi
}

#
# search a directory recursively for $CONTROL files
#
# paramters:
# $1 directory to search for modules
#
find_modules() {
  if test -d "$1"; then
    local dir=$(cd "$1" && pwd)
    while read m; do
      if test "x$m" != "x"; then
        export module=""
        parse_control "$m"
        export MODULES="$MODULES $module"
      fi
    done <<EOF
 $(find "$dir" -name $CONTROL )
EOF
  else
    echo "ERROR: could not find directory $1"
    false
  fi
}

#
# sort $MODULES according to the dependencies
#
sort_modules() {
  for m in "$@"; do
    _sort_module $m
  done
  if test x"$ONLY" != x; then
	SORTEDMODULES=$ONLY
  fi
  export MODULES=$SORTEDMODULES
}

#
# recursive part of sort_modules
# evaluate dependencies of one module
#
# paramters:
# $1 name of the modules
#
_sort_module() {
  local module=$1
  shift 1
  if ! check_modname $module; then echo "ERROR: invalid module name $module"; exit 1; fi
  if test "x$(eval echo \$SORT_DONE_${command}_${module})" != "xyes"; then
    # resolve dependencies
    for dep in $(eval "echo \$DEPS_$module"); do
      if ! check_modname $dep; then echo "ERROR: invalid module name $dep"; exit 1; fi
      if test "x$(eval echo \$HAVE_$dep)" != "x"; then
        _sort_module $dep
      else
        echo "ERROR: could not find module $dep"
        exit 1
      fi
    done
    # resolve suggestions
    for dep in $(eval "echo \$SUGS_$module"); do
      if ! check_modname $dep; then echo "ERROR: invalid module name $dep"; exit 1; fi
      if test "x$(eval echo \$HAVE_$dep)" != "x"; then
        _sort_module $dep
      fi
    done
    # insert this module into the list
    export SORT_DONE_${command}_${module}=yes
    export SORTEDMODULES="$SORTEDMODULES $module"
  fi
}

#
# load the $CONTROL file, skip all control variables
# and run a command
#
# parameters:
# $1 command to execute
# $2 full path of the $CONTROL file
#
eval_control() {
  local command=$1
  local file=$2
  shift 2
  if test -f "$file"; then
    # open subshell
    (
      set -e
      # load functions defined in $file
      # if $command is not defined in $file,
      # then the default implementation will be executed
      eval "$(grep -v -e '^[[:alnum:]]\+:' $file)"
      # execute $command
      $command
    ) || false
  else
    echo "ERROR: could not find $file"
    exit 1
  fi
}


#
# make sure the module name fits the naming convention
#
# parameters:
# $1 module name
#
check_modname() {
  if ! echo "$1" | grep -q '^[a-zA-Z0-9_]\+$'; then
    return 1
  fi
  return 0
}