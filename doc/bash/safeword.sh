_safeword()
{
	local cur prev opts args
	COMPREPLY=()
	cur="${COMP_WORDS[COMP_CWORD]}"
	prev="${COMP_WORDS[COMP_CWORD-1]}"
	subcommand="${COMP_WORDS[1]}"

	#
	# Complete the subcommands
	#
	opts="--version"
	subcommands="help init add ls tag cp"

	#
	# Complete arguments for the subcommands
	#
	case "${subcommand}" in
	help)
		COMPREPLY=( $(compgen -W "${subcommands}" -- ${cur}) )
		;;
	init)
		opts="--force"
		COMPREPLY=( $(compgen -f -d  -W "${opts}" -- ${cur}) )
		;;
	add)
		opts="--message --tag"
		COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
		;;
	ls)
		opts="--all"
		local tags=$( safeword tag )
		COMPREPLY=( $(compgen -W "${opts} ${tags}" -- ${cur}) )
		;;
	tag)
		opts="--delete --force --move --wiki"
		case "${prev}" in
		tag)
			local credentials=$( safeword ls --all | cut -d' ' -f1 )
			COMPREPLY=( $(compgen -W "${credentials} ${opts}" -- ${cur}) )
			;;
		--wiki | -w)
			COMPREPLY=( $(compgen -f -d  -W "-" -- ${cur}) )
			;;
		*)
			local tags=$( safeword tag )
			COMPREPLY=( $(compgen -W "${tags} ${opts}" -- ${cur}) )
			;;
		esac
		;;
	cp)
		opts="--time --username --password"
		case "${prev}" in
		cp)
			local credentials=$( safeword ls --all | cut -d' ' -f1 )
			COMPREPLY=( $(compgen -W "${credentials} ${opts}" -- ${cur}) )
			;;
		*)
			COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
			;;
		esac
		;;
	*)
		COMPREPLY=( $(compgen -W "${opts} ${subcommands}" -- ${cur}) )
		;;
	esac

	return 0
}
complete -F _safeword safeword