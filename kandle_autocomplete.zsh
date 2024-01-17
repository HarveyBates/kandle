#compdef kandle

_arguments \
  '1:Commands:->cmds' \
  '*::arg:->args'

_contains_arg() {
  local arg="$1"
  for word in "${words[@]}"; do
    if [[ "$word" == "$arg" ]]; then
      return 0
    fi
  done
  return 1
}

case "$state" in
  cmds)
    local -a commands
    commands=(
      'init:Initialize kandle directory structure'
      '-I:Initialize kandle directory structure'
      'list:List libraries in project'
      '-L:List libraries in project'
      'filename:Downloaded .zip filename'
      '-f:Downloaded .zip filename'
      'library:Specify a component library name (e.g. n-channel-mosfet)'
      '-l:Specify a component library name (e.g. n-channel-mosfet)'
      'help:Show help'
      '-h:Show help'
    )
    _describe 'Commands' commands
    ;;
  args)
    if _contains_arg "library" || _contains_arg "-l"; then
      # Search libraries (just looking at symbols)
      local symbols_dir="$PWD/components/extern/symbols"
      if [[ -d $symbols_dir ]]; then
        # Get symbol files, only shows the filename (not extention due to (:r))
        _path_files -W $symbols_dir -g '*(.kicad_sym)(:r)'
      else
        # Internal message (not shown to user)
        _message "Symbol directory not found in current working directory."
      fi
    else
      # Handle general files and directories tab completion
      _files -/ -g '*'
    fi
    ;;
esac
