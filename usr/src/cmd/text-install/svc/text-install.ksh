#!/bin/ksh
#
# plain text installer for illumos distributions,
# not suitable for SunOS 4.x, or older.
#
# Copyright (c) 2025, 2026 Klaus Ziegler
#
# Lector/Alpha tests: Volker A. Brandt
#
# netmask validation/function by Olaf Bohlen Oct 2025
#

trap 'rm -f /tmp/*.log; exit' 0 1 2 15

biosboot()
{
    rm /tmp/vtoc.$$
    print "Initial disk drive layout:" >>$LOG
    printf "* Id    Act  Bhead  Bsect  Bcyl    Ehead  " >>$LOG
    print "Esect  Ecyl    Rsect      Numsect" >>$LOG
    /sbin/fdisk -W -v /dev/rdsk/${disk}p0 | \
	/usr/bin/egrep -v "^\*|^$" | sed -e "/^  0 /d" >>$LOG
    print "Agenda for Id:" >>$LOG
    print "*   7: Windows / reserved BIOS/MSR/NTFS" >>$LOG
    print "*  15: Extended partition" >>$LOG
    print "* 130: Linux swap / old Solaris" >>$LOG
    print "* 131: Linux Native ext4 / xfs" >>$LOG
    print "* 142: Linux LVM physical volume\n" >>$LOG
    # cyl used.
    cyuse=`awk '$1!=0{s+=$10};END{print s}' /tmp/fdisk_table.$$`
    p0sta=`head -1 /tmp/fdisk_table.$$ | awk '{print $9}'`
    cyuse=`expr $cyuse + $p0sta`
    ablk=`/usr/bin/diskinfo -H -p | awk '$2 ~ /'"$disk"'/{s=$5/512}END{print s}'`
    log +tc "The choosen disk drive: $disk is: "
    dsksize $disk
    log "$showon in size."

    if [ "$illu" != "yes" ]; then
	# check whats installed.
	part=0
	cat /tmp/fdisk_table.$$ | while read id act bhead bsec bcl ehead esec ecyl rsec nsec
	do
	    case $id in
		7)
		    if [ -n "$win" ]; then
			wpart=$wpart 
			part=`expr $part + 2 `
		    else
			win=yes
			wpart=`expr $part + 1`
		    fi ;;
		131)
		    if [ -n "$lpart" ]; then
			part=`expr $part + 1`
		    else
			lin=yes
			lpart=`expr $part + 1`
		    fi ;;
		142)
		    lvm=yes lvmp=$part
		    part=`expr $part + 1` ;;
		*)
		    part=`expr $part + 1 ` ;;
	    esac
	done

	log +tc "Other operating systems use: "
	if [ "$ablk" -le "$cyuse" ]; then
	    reclaim=yes
	    log "all available space!"
	else
	    if [ "$cyuse" -eq 0 ]; then
		log "0MB"
	    else
		avail=`echo $cyuse | awk '{s=($1*512)/1024/1024/1024}END{printf("%.2fGB\n"), s}'`
		log "$avail"
	    fi
	fi

	if [ "$numuse" -ge 3 -a "$lin" = "yes" -o "$reclaim" = "yes" ]; then
	    unset lin
	    print "\n\tLinux Operating System on partition: $lpart found. This leaves no partition"
	    print "\tfor ${distribution}, however Linux can be started from a logical partition."
	    print "\tDo you want to resuse this space for ${distribution} installation?"
	    print "\tOf course this means you'll have to reinstall your Linux distribution."
	    log +ntc "Reuse space for ${distribution} installation (default=Y/n?): "
	    if yesno; then
		log +tc "Removing current Linux installation..."
		if [ "$win" = "yes" ]; then
		    lblk=`awk '$1!=7{s+=$10};END{print s}' /tmp/fdisk_table.$$`
		    grep "^  7 " /tmp/fdisk_table.$$ >/tmp/fdisk_table
		    mv /tmp/fdisk_table /tmp/fdisk_table.$$
		    numuse=`/usr/bin/egrep -c -v "^  0|^  191" /tmp/fdisk_table.$$`
		    cyuse=`expr $cyuse - $lblk`
		    ablk=`expr $ablk - $cyuse`
		    unset avail
		    /sbin/fdisk -F /tmp/fdisk_table.$$ /dev/rdsk/${disk}p0
		else
		    echo "  0 0 0 0 0 0 0 0 0 0" >/tmp/fdisk_table.$$
		    echo "  0 0 0 0 0 0 0 0 0 0" >>/tmp/fdisk_table.$$
		    echo "  0 0 0 0 0 0 0 0 0 0" >>/tmp/fdisk_table.$$
		    echo "  0 0 0 0 0 0 0 0 0 0" >>/tmp/fdisk_table.$$
		    cyuse=0 numuse=0
		    /sbin/fdisk -F /tmp/fdisk_table.$$ /dev/rdsk/${disk}p0
		    rm /tmp/fdisk_table.$$
		fi
		log " done"
	    else
		printf "\n\tUnable to install, no free primary fdisk partition available - exit."
		sleep 10; exit 0
	    fi
	fi

	if [ "$win" = "yes" ]; then
	    echo ""
	    log +tc "Windows Operating System installation found using: "
	    winsz=`awk '/^  7 / {s=(s1+=$10*512)/1024/1024/1024}; \
		END {printf("%.2f\n"), s}' /tmp/fdisk_table.$$`
	    log "${winsz}GB"
	    showsz=`echo $showsz - $winsz | bc`
	    log +t "Windows Operating System boot on partition: $wpart found."
	    if [ "$cyuse" -gt "$ablk" ]; then
		print "\tIt uses the complete disk, please shrink it or re-install, leave at"
		printf "\tleast a minimum of 10GB for $distribution - press <RETURN> to exit."
		read go
		exit 0
	    else
		log +t "Adding Boot-Manager entry for Windows: (chain disk0s${wpart}:)."
		echo "title Windows" >> /tmp/menu.lst
		echo "chain disk0s${wpart}:" >> /tmp/menu.lst
	    fi
	fi

	if [ "$lin" = "yes" -a -z "$win" ]; then
	    log +t "Linux Operating System boot on partition: $lpart found."
	    if [ "$lvm" = "yes" ]; then
		log +t "Logical Volume Manager for Linux on partition: $lvmp found."
	    fi
	    log +tc "Adding Boot-Manager entry: Linux (chain disk0s${lpart}:)..."
	    echo "title Linux" >> /tmp/menu.lst
	    echo "chain disk0s${lpart}:" >> /tmp/menu.lst
	    log " done"
	fi

	[ -z "$lblk" ] && ablk=`expr $ablk - $cyuse`
	if [ "$ablk" -gt "$cyuse" ]; then
	    if [ "$reclaim" = "yes" ]; then
		if [ "$win" = "yes" ]; then
		    log +t "Configure a Tripple-Boot setup with Windows & Linux!"
		    log +t "This requires you to install the GRUB boot-loader into the MBR,"
		    log +t "while installing the Linux distribution. The needed GRUB Boot-Manager"
		    log +t "entry for $distribution can be found in the install log directory:"
		    log +t "/var/sadm/system/logs named grub.cfg after installation.\n"
		    print "menuentry '$distribution Boot Manager (on /dev/sda3)' {" >/tmp/grub.cfg
		    print "\tinsmod part_msdos\n\tset root='hd0,3'\n\tchainloader +1\n}" >>/tmp/grub.cfg
		    stcyl=`sed -e "/^  0 /d" /tmp/fdisk_table.$$ |  tail -1 | \
			awk '{s=$9+$10};END {printf("%-10d"), s}'`
		    bcnt=`expr $ablk - $cyuse`
		else
		    bcnt=0
		    log +tc "Adding Boot-Manager entry: Linux (chain disk0s5:)..."
		    echo "title Linux" >> /tmp/menu.lst
		    echo "chain disk0s5:" >> /tmp/menu.lst
		    log " done"
		fi
		loop=true
		while [ "$loop" != "done" ]; do
		    do_size Linux $bcnt
		done
		log +t "The reserved size for the Linux installation is: $want"
	    fi
	    avail=`echo $ablk | awk '{s=($1*512)/1024/1024/1024}END{printf("%.2fGB\n"), s}'`
	    log +nt "Allocating the rest of $avail for: ${distribution} installation."
	    printf "\tCreating a new primary partition for: ${distribution}"
	    sed -e "/^  0/d" /tmp/fdisk_table.$$ >/tmp/fdisk_table
	    stcyl=`tail -1 /tmp/fdisk_table | awk '{s=$9+$10};END {printf("%-10d"), s}'`
	    printf "  191   0    0      0      0       0      0" >>/tmp/fdisk_table
	    print "      0       $stcyl $ablk" >>/tmp/fdisk_table
	    /sbin/fdisk -F /tmp/fdisk_table /dev/rdsk/${disk}p0
	    print "Disk partition layout after new label:" >>$LOG
	    /sbin/fdisk -W -v /dev/rdsk/${disk}p0 | /usr/bin/egrep -v "^\*|^$" | \
		sed -e "/^  0 /d" >>$LOG
	    printf "."
	    echo "p\np\nq\n" | /usr/sbin/format -d ${disk} >/tmp/disksz.log 2>/dev/null
	    echo "${num}\nq\n" | /usr/sbin/format | grep cyl >/tmp/dtype.log 2>/dev/null
	    ftyp=`nawk '{gsub("<",""); print $3}' /tmp/dtype.log`
	    dtyp=`dmesg | grep "Disk$num" | sed \
		-e "s@^.*uct '@@g" \
		-e "s@ '.*@@g" \
		-e "s@'>@@g" \
		-e "s@ @-@g" | sort -u`
	    dtype=`echo $ftyp | sed -e "s@Unknown-Unknown@$dtyp@g"`
	    cyl=`awk '{print $5}' /tmp/dtype.log`
	    head=`awk '{print $9}' /tmp/dtype.log`
	    sect=`nawk '{gsub(">",""); print $11}' /tmp/dtype.log`
	    printf "."
	    s0s=`awk '/^  8|^  9/ {s=$4+$6}END{print s}' /tmp/disksz.log`
	    s8e=`awk '/^  8/ {print $9}' /tmp/disksz.log`
	    s9e=`awk '/^  9/ {print $9}' /tmp/disksz.log`
	    s2l=`awk '/^  2/ {print $9}' /tmp/disksz.log`
	    s0l=`expr $s2l - $s9e - $s8e`
	    printf "."
	    printf "t\n1\n${cyl}\n2\n\n${head}\n${sect}\n" >/tmp/format_script
	    printf "\n\n\n\n\n\n\n\n\n\n\n${dtype}\np\n0\nroot\n\n" >>/tmp/format_script
	    print "${s0s}\n${s0l}b\nl\nq\n" >>/tmp/format_script
	    /usr/sbin/format -s -d ${disk} -f /tmp/format_script 2>/dev/null
	    print "Disk type after new disk label:" >>$LOG
	    print "curr\nq\n" | /usr/sbin/format -d ${disk} | grep cyl 2>&1 >>$LOG
	    echo " done"
	    zslice=0
	    do_zpool
	else
	    print "\tNot enough space left on disk: ${disk} for ${distribution} "
	    sleep 10; exit 0
	fi
	rm -f /tmp/fdisk_table* /tmp/dtype.log /tmp/format_script /tmp/disksz.log /tmp/new_label.*
    fi
    defpart=no
    disk_okay=okay
}

dial()
{
    pid=$1
    state=0
    while [ -d /proc/$pid ]; do
	case $state in
	    0) echo "|\b\c"; state=1 ;;
	    1) echo "/\b\c"; state=2 ;;
	    2) echo "-\b\c"; state=3 ;;
	    3) echo "\\" "\b\b\c"; state=0 ;;
	esac
	sleep 1
    done
}

dsksize()
{
    szspec=`/usr/bin/diskinfo -H | awk '$2 ~ /'"$1"'/{print $6}'`
    case $szspec in
	MiB) SZ=MB ;;
	GiB) SZ=GB ;;
	TiB) SZ=TB ;;
    esac
    showsz=`/usr/bin/diskinfo -H | awk '$2 ~ /'"$1"'/{print $5}'`
    showon="${showsz}${SZ}"
}

# function to extract clear-text strings from DHCP verndor option 43
extract_subopt()
{
    typeset target_code=$1
    typeset hex=$HEX_DATA
    typeset code len_hex len_dec val_hex text byte i
    
    while [ ${#hex} -gt 0 ]; do
	code=${hex:0:2}
	len_hex=${hex:2:2}
	len_dec=$((16#$len_hex))
	val_hex=${hex:4:$((len_dec * 2))}
        
	if [ "$code" = "$target_code" ]; then
	    text=""
	    for ((i=0; i<${#val_hex}; i+=2)); do
		byte=${val_hex:$i:2}
		text="${text}$(printf "\\x$byte")"
	    done
	    echo "$text"
	    return 0
	fi
        
	hex=${hex:$((4 + len_dec * 2))}
    done
    return 1
}

grub_cfg()
{
    gdir=/tmp/pcfs/EFI/$distribution
    printf "menuentry '" >$gdir/grub.cfg
    printf "$distribution " >>$gdir/grub.cfg
    printf "Boot Manager (" >>$gdir/grub.cfg
    printf "on /dev/" >>$gdir/grub.cfg
    if [ `expr "$disk" : '.* *'` -gt "7" ]; then
	dskclass=nvme0n1
    else
	dskclass=sd0
    fi
    lxefi=`expr $efis + 1`
    printf "${dskclass}p${lxefi})' " >>$gdir/grub.cfg
    printf "--class windows --class " >>$gdir/grub.cfg
    printf 'os $menuentry_id_option ' >>$gdir/grub.cfg
    print "'osprober-efi-0000-0000' {" >>$gdir/grub.cfg
    print "\tinsmod part_gpt" >>$gdir/grub.cfg
    printf "\tsearch --no-floppy --fs-uuid " >>$gdir/grub.cfg
    echo '--set=root 0000-0000' >>$gdir/grub.cfg
    printf "\tchainloader /EFI/" >>$gdir/grub.cfg
    printf "${distribution}/" >>$gdir/grub.cfg
    print "Boot/bootx64.efi\n}" >>$gdir/grub.cfg

    print "#!/bin/sh\n#" >$gdir/add_grub
    printf 'printf "Adding: ' >>$gdir/add_grub
    printf "$distribution" >>$gdir/add_grub
    print " Boot-Manager to your grub.cfg...\"" >>$gdir/add_grub
    printf "awk '/^#$/,/30_os-prober/' " >>$gdir/add_grub
    print '/boot/grub/grub.cfg > \' >>$gdir/add_grub
    print "\t/tmp/grub_cfg" >>$gdir/add_grub
    printf "cat /boot/efi/EFI/" >>$gdir/add_grub
    printf "$distribution" >>$gdir/add_grub
    print "/grub.cfg >>/tmp/grub_cfg" >>$gdir/add_grub
    print 'winin=`grep -c chainloader /boot/grub/grub.cfg`' >>$gdir/add_grub
    print 'if [ "$winin" -ge 1 ]; then' >>$gdir/add_grub
    printf "\tawk '/Windows Boot Manager/,/END && 41_custom/' " >>$gdir/add_grub
    print '/boot/grub/grub.cfg >> \' >>$gdir/add_grub
    print "\t/tmp/grub_cfg\nelse" >>$gdir/add_grub
    print '\techo "set timeout_style=menu" >> /tmp/grub_cfg' >>$gdir/add_grub
    printf "\techo 'if [ " >>$gdir/add_grub
    printf '"${timeout}" ' >>$gdir/add_grub
    print "= 0 ]; then' >> /tmp/grub_cfg" >>$gdir/add_grub
    print '\techo "  set timeout=10" >> /tmp/grub_cfg' >>$gdir/add_grub
    print '\techo "fi" >> /tmp/grub_cfg' >>$gdir/add_grub
    print ' \techo "### END /etc/grub.d/30_os-prober ###" >> /tmp/grub_cfg' >>$gdir/add_grub
    printf "\tawk '/30_uefi-firmware/,/END && 41_custom/' " >>$gdir/add_grub
    print '/boot/grub/grub.cfg >> \' >>$gdir/add_grub
    print "\t/tmp/grub_cfg\nfi" >>$gdir/add_grub
    print "mv /tmp/grub_cfg /boot/grub/grub.cfg" >>$gdir/add_grub
    print "chmod 0600 /boot/grub/grub.cfg" >>$gdir/add_grub
    print 'echo " done"' >>$gdir/add_grub
    print "/usr/sbin/grub-install" >>$gdir/add_grub
    chmod 0777 $gdir/add_grub
}

do_size()
{
    if [ -n "$avail" ]; then
	availsz=`echo $avail | sed -e "s@[GTM]B@@g"`
	showsz=`echo $showsz - $availsz | bc`
    fi
    while true
    do
	printf "\tHow much space to allocate for $1 - ${showsz}${SZ} max: "
	read want
	case $want in
	    *.*)
		print "\tonly use even numbers please - try again."
	    ;;
	    [0-9]*[m,b]|[0-9]*[g,b]|[0-9]*[t,b])
		val=`echo $want | sed -e "s@[0-9]*@@g"`
		wsz=`echo $want | sed -e "s@[m,g,t]b@@g"`
		case $val in
		    mb) blks=`echo $wsz | awk '{s=($1*1024*1024)/512;print s}'`
		    ;;
		    gb) blks=`echo $wsz | awk '{s=($1*1024*1024*1024)/512;print s}'`
		    ;;
		    tb) blks=`echo $wsz | awk '{s=($1*1024*1024*1024*1024)/512;print s}'`
		    ;;
		esac
		#if [ "$ablk" -le $2 ]; then - before BIOS multiboot.
		if [ "$ablk" -gt $2 ]; then
		    ablk=`expr $ablk - $blks`
		    echo "$want" >>/tmp/new_label.$$
		    loop=done
		    break
		else
		    print "\t$want too big - try again."
		fi
	    ;;
	    *)
		print "\tg(gigabytes), m(megabytes) or t(terrabytes) please - try again."
	    ;;
	esac
    done
}

do_zpool()
{
    if [ -n "$zslice" ]; then
	printf "\tCreating root zpool: rpool on ${disk}s${zslice}..."
	zpool create -f rpool /dev/dsk/${disk}s${zslice}
    else
	printf "\tCreating $PART partition and zpool: rpool..."
	zpool create -fB rpool /dev/dsk/${disk}
	zslice=1
    fi
    pool_name=rpool
    defpart=no
    nozpool=yes
    print " done"
}

noyes()
{
    while true
    do
	read response
	case "$response" in
	    [Yy]|[Yy][Ee][Ss])
		return 1
	    ;;
	    [Nn]|[Nn][Oo]|"")
		return 0
	    ;;
	    *)
		printf "\t\tPlease answer yes (y) or no (n): "
	    ;;
	esac
    done
}

# function to prepare x86 disk, only used interactively.
efiboot()
{
    if [ $numlen -gt 1 ]; then
	printf "\n\tSorry - Multiboot Setup for mirrored disks is not supported."
	sleep 5; return
    fi
    dsksize $disk
    [ -z "$resv" ] && resv=no
    olayout=yes
    # test for unconfigured disk.
    if [ ! -s /tmp/vtoc.$$ ]; then
	print "The disk had no defined partition table entries!" >>$TLOG
	olayout=no
	/sbin/zpool create -fB rpool /dev/dsk/$disk
	/sbin/zpool destroy rpool
    fi
    echo "p\np\nq\n" | /usr/sbin/format -d ${disk} >/tmp/disksz.log 2>/dev/null
    /usr/sbin/prtvtoc -h /dev/rdsk/${disk}s2 >/tmp/vtoc.$$
    if [ -s /tmp/vtoc.$$ ]; then
	grep "^  [0-9]" /tmp/disksz.log | sed \
	    -e '/unassigned/d' >/tmp/disklabel.orig
	ronly=`head -1 /tmp/disklabel.orig | awk '{print $1}'`
	[ "$ronly" = 8 ] && resv=no
    fi
    ablk=`awk '/^Total/{s=$5+$7+33};END{print s}' /tmp/disksz.log`
    # ask for multiboot setup:
    # 1. if reserved slice is 'no' - blank disk.
    # 2. if reserved slice is there, but the only one.
    # 3. if illumos ZFS is on slice 1 and is of type usr.
    illu=`awk '$1==1 && $2==4 {print "yes"}' /tmp/vtoc.$$`
    if [ "$resv" = "no" -o "$illu" = "yes" ]; then
	print "\n\t\t*** Multiboot Install Instructions ***\n"
	print "\tTo enable Multiboot, disk drive: $disk needs to be"
	print "\tpartioned, this will destroy all data. The installations for other"
	print "\toperating systems must be done in the following order:\n"
	print "\t1. Prepare partitions for a Windows Installation and reboot."
	print "\t   In the Windows installer highlight partition 3 and format it."
	print "\t   Do NOT change the layout at all, make sure that partition 3"
	print "\t   is still highlighted before you select next."
	print "\t2. Come back and restart this installer, then you can decide"
	print "\t   how much space to spent for  ${distribution}/Linux."
	print "\tNote:"
	print "\tTo boot ${distribution} you will need a boot-manager, or an UEFI boot"
	print "\tapplication, which can usually be found in your system BIOS.\n"
	if [ -n "$efis" -a -n "$msr" -a -n "$ntfs" ]; then
	    print "\tDetected a Windows installation using the complete disk, this"
	    print "\tcan't be used to install ${distribution} operating system! Selecting"
	    print "\tone of below options, will erase this Windows installation.\n"
	fi
	print "\tSelect one of the following Multiboot Setups:\n"
	print "\t\t1 $distribution + Linux"
	print "\t\t2 $distribution + Windows"
	print "\t\t3 $distribution + Windows + Linux"
	print "\t\tq back to disk selection\n"
	printf "\t\tYour choice: "
	printf "\nMultiboot Setup Option: " >>$TLOG
	while true
	do
	    read choice
	    case $choice in
		1) print "$distribution + Linux has been selected" >>$TLOG
		    needlin=yes  break 1;;
		2) print "$distribution + Windows has been selected" >>$TLOG
		    needwin=yes  break 1;;
		3) print "$distribution + Windows + Linux has been selected" >>$TLOG
		    needwin=yes needlin=yes break 1 ;;
		q) return ;;
		*) echo "\n\t\twrong input - try again.\n" ;;
	    esac
	done
    fi

    if [ "$needwin" = "yes" ]; then
	if [ "$olayout" = "yes" ]; then
	    grep "^  [0-9]" /tmp/disksz.log | sed \
		-e '/unassigned/d' \
		-e "s@0     system@0       UEFI@g" \
		-e "s@1          -@1        MSR@g" \
		-e "s@2          -@2       NTFS@g" >/tmp/disklabel.orig
	fi
	/sbin/zpool create -fB rpool /dev/dsk/$disk
	/sbin/zpool destroy rpool
	echo "p" >/tmp/new_label.$$
	print "0\nsystem\nwm\n2048\n100mb" >>/tmp/new_label.$$
	print "1\nroot\nwm\n206848\n16mb" >>/tmp/new_label.$$
	print "2\nhome\nwm\n239616" >>/tmp/new_label.$$
	print "\n\tNow you need to resize your Windows Installation."
	bcnt=`expr $ablk - 237568`
	loop=true
	while [ "$loop" != "done" ]; do
	    do_size Windows $bcnt
	done
	echo "l\nq\n" >>/tmp/new_label.$$
	/usr/sbin/format -s -d ${disk} -f /tmp/new_label.$$ 2>/dev/null
	echo "p\np\nq\n" | /usr/sbin/format -d ${disk} >/tmp/disksz.log 2>/dev/null
	/usr/sbin/prtvtoc -h /dev/rdsk/${disk}s2 >/tmp/vtoc1.$$
	# reset IDs to be Windows compatible.
	sed -e "s@ 2    00@19    00@g" -e "s@ 8    00@20    00@g" /tmp/vtoc1.$$ >/tmp/vtoc.$$
	/usr/sbin/fmthard -s /tmp/vtoc.$$ /dev/rdsk/${disk}s2 >/dev/null 2>/dev/null
	while true
	do
	    echo y
	done | /usr/sbin/mkfs -F pcfs /dev/rdsk/${disk}s0
	[ ! -d /tmp/pcfs ] && mkdir /tmp/pcfs
	mount -F pcfs /dev/dsk/${disk}s0 /tmp/pcfs
	mkdir -p /tmp/pcfs/EFI/${distribution}/Boot
	grep "^  0" /tmp/disksz.log | sed \
	    -e "s@0     system@0       UEFI@g" >/tmp/pcfs/EFI/${distribution}/disklabel.win
	grep "^  1" /tmp/disksz.log | sed \
	    -e "s@1       root@1        MSR@g" >>/tmp/pcfs/EFI/${distribution}/disklabel.win
	grep "^  2" /tmp/disksz.log | sed \
	    -e "s@2          -@2       NTFS@g" >>/tmp/pcfs/EFI/${distribution}/disklabel.win
	grep "^  8" /tmp/disksz.log >>/tmp/pcfs/EFI/${distribution}/disklabel.win
	rm /tmp/vtoc* /tmp/new_label.$$ /tmp/disksz.log
	if [ -f /tmp/disklabel.orig ]; then
	    mv /tmp/disklabel.orig /tmp/pcfs/EFI/${distribution}
	fi
	mv $TLOG /tmp/pcfs/EFI/${distribution}
	cp -p /boot/loader64.efi /tmp/pcfs/EFI/${distribution}/Boot/bootx64.efi
	cp -p /boot/loader32.efi /tmp/pcfs/EFI/${distribution}/Boot/bootia32.efi
	if [ "$needlin" = "yes" ]; then
	    touch /tmp/pcfs/EFI/${distribution}/grub.cfg
	fi
	(cd /tmp/pcfs; chmod -R 0777 EFI/${distribution})
	umount /tmp/pcfs
	print "\n\tRebooting for Windows Install...\n"
	/usr/sbin/reboot
    fi

    msr=`awk '$2==19 {print "yes"}' /tmp/vtoc.$$`
    ntfs=`awk '$2==20 {print "yes"}' /tmp/vtoc.$$`
    if [ "$resv" = "yes" -a -n "$efis" -a "$msr" = "yes" -a "$ntfs" = "yes" ]; then
	# must be our prepared Windows installation.
	zslice=3
	ablk=`awk '/^Total/{s=$5+$7+33-2047};END{print s}' /tmp/disksz.log`
	print "\n\tWelcome back to $distribution installation, the selected Windows"
	printf "\tlayout uses: "
	bcnt=`awk '{s+=$5};END{print s}' /tmp/vtoc.$$`
	wingb=`echo $bcnt | awk '{s=($1 * 512)/1024/1024/1024; printf ("%.2fGB\n"), s}'`
	printf "$wingb out of: "
	printf "$showsz leaving: "
	bcnt=`expr $ablk - $bcnt`
	showsz=`echo $bcnt | awk '{s=($1 * 512)/1024/1024/1024; printf ("%.2fGB\n"), s}'`
	echo $showsz
	mkdir /tmp/pcfs
	mount -F pcfs /dev/dsk/${disk}s${efis} /tmp/pcfs
	# check if user requested Linux
	if [ -f /tmp/pcfs/EFI/${distribution}/disklabel.orig ]; then
	    mv /tmp/pcfs/EFI/${distribution}/disklabel.orig /tmp
	else
	    rm -f /tmp/disklabel.orig
	fi
	cp /tmp/pcfs/EFI/${distribution}/disklabel.win /tmp
	if [ -f /tmp/pcfs/EFI/${distribution}/migration_log ]; then
	    cp -p /tmp/pcfs/EFI/${distribution}/migration_log $TLOG
	fi
	if [ -f /tmp/pcfs/EFI/${distribution}/grub.cfg ]; then
	    needlin=yes
	    endmsg=Linux
	else
	    endmsg=$distribution
	fi
	if [ -f /tmp/pcfs/EFI/${distribution}/disklabel.orig ]; then
	    cp -p /tmp/pcfs/EFI/${distribution}/disklabel.orig /tmp
	fi
	umount /tmp/pcfs
	start=`awk '$2==20 {print $6+1}' /tmp/vtoc.$$`
	print "p\n${zslice}\nusr\nwm\n${start}" >>/tmp/new_label.$$
	if [ "$needlin" = "yes" ]; then
	    loop=true
	    while [ "$loop" != "done" ]; do
		do_size $distribution $bcnt
	    done
	    next=`expr $zslice + 1`
	    start=`expr $start + $blks`
	    print "${next}\nroot\nwm\n${start}" >>/tmp/new_label.$$
	    rstart=`awk '$2==11 {print $4}' /tmp/vtoc.$$`
	    bcnt=`expr $rstart - $start`
	fi
	printf "\tAllocating rest of: "
	showsz=`echo $bcnt | awk '{s=($1 * 512)/1024/1024/1024; printf ("%.2fGB\n"), s}'`
	printf "$showsz for ${endmsg}..."
	print "${bcnt}b\nl\nq\n" >>/tmp/new_label.$$
	cp /tmp/vtoc.$$ /tmp/vtoc.bak
	/usr/sbin/format -s -d ${disk} -f /tmp/new_label.$$ 2>/dev/null
	print " done"
	if [ -f $TLOG ]; then
	    cat $TLOG >>$LOG
	fi
	print "\nNew partition table for Windows installation:" >>$LOG
	if [ -f /tmp/disklabel.orig ]; then
	    print "\nOriginal partition table:" >>$LOG
	    cat /tmp/disklabel.orig >>$LOG
	    print "\n\t\t\tOld partition layout:"
	    print "\tPart      Tag    Flag     First Sector          Size          Last Sector"
	    awk '{print "\t" $0}' /tmp/disklabel.orig
	fi
	print "\nFinal partition table:" >>$LOG
	print "\n\t\t\tNew partition layout:"
	print "\tPart      Tag    Flag     First Sector          Size          Last Sector"
	echo "n\np\np\nq\n" | /usr/sbin/format -d ${disk} >/tmp/disksz.log 2>/dev/null
	grep "^  [0-9]" /tmp/disksz.log | sed \
	    -e '/unassigned/d' \
	    -e "s@0     system@0       UEFI@g" \
	    -e "s@1          -@1        MSR@g" \
	    -e "s@2          -@2       NTFS@g" \
	    -e "s@3        usr@3        ZFS@g" \
	    -e "s@4       root@4      Linux@g" >/tmp/final.label
	awk '{print "\t" $0}' /tmp/final.label
	cat /tmp/final.label >>$LOG
	printf "\n\tOkay to use this partition layout (default=Y/n?):"
	if yesno; then
	    do_zpool
	    disk_okay=okay
	    return
	else
	    /usr/sbin/fmthard -s /tmp/vtoc.bak /dev/rdsk/${disk}s2 >/dev/null 2>/dev/null
	    return
	fi
    fi

    if [ "$needlin" = "yes" ]; then
	efis=0
	echo
	if [ -f /tmp/disklabel.orig ]; then
	    print "Original disk label before $distribution / Linux installation:" >>$LOG
	    cat /tmp/disklabel.orig >>$LOG
	fi
	[ `mount | grep -c /tmp/pcfs` -eq 1 ] && umount /tmp/pcfs
	/usr/sbin/prtvtoc -h /dev/rdsk/${disk}s2 >/tmp/vtoc.$$
	[ ! -s /tmp/vtoc.$$ ] && /sbin/fdisk 0:0 /dev/rdsk/$disk
	/usr/sbin/prtvtoc -h /dev/rdsk/${disk}s2 >/tmp/vtoc.$$
	print "p\n0\nsystem\nwm\n256\n256mb" >>/tmp/new_label.$$
	start=524544
	ablk=`awk '/^Total/{s=$5+$7+33-2047};END{print s}' /tmp/disksz.log`
	bcnt=`expr $ablk - $start`
	zslice=`awk '$2==4 {print $1}' /tmp/vtoc.$$`
	[ -z "$zslice" ] && zslice=1
	print "${zslice}\nusr\nwm\n${start}" >>/tmp/new_label.$$
	loop=true
	while [ "$loop" != "done" ]; do
	    do_size $distribution $bcnt
	done
	next=`expr $zslice + 1`
	start=`expr $start + $blks`
	print "${next}\nroot\nwm\n${start}" >>/tmp/new_label.$$
	rstart=`awk '$2==11 {print $4}' /tmp/vtoc.$$`
	bcnt=`expr $rstart - $start`
	printf "\tAllocating rest of: "
	showsz=`echo $bcnt | awk '{s=($1 * 512)/1024/1024/1024; printf ("%.2fGB\n"), s}'`
	printf "$showsz for Linux..."
	print "${bcnt}b\nl\nq\n" >>/tmp/new_label.$$
	/usr/sbin/format -s -d ${disk} -f /tmp/new_label.$$ 2>/dev/null
	print " done"
	do_zpool
	disk_okay=okay
    fi
}

log()
{
    [ "$1" = "+t" ] && shift && T=1 oflag="\t$*"
    [ "$1" = "+c" ] && shift && C=1 oflag="$*\c"
    [ "$1" = "+nt" ] && shift && N=1 oflag="\n\t$*"
    [ "$1" = "+nc" ] && shift && NT=1 oflag="\n$*\c"
    [ "$1" = "+ntc" ] && shift && NTC=1 oflag="\n\t$*\c"
    [ "$1" = "+tc" ] && shift && C=1 oflag="\t$*\c"
    if [ -n "$oflag" ]; then
	echo "${oflag}" 1>&2
	[ -n "$NTC" ] && echo "\n$*\c" >>$LOG
	[ -n "$NC" ] && echo "\n$*\c" >>$LOG
	[ -n "$N" ] && echo "\n$*" >>$LOG
	[ -n "$C" ] && echo "$*\c" >>$LOG
	[ -n "$T" ] && echo "$*" >>$LOG
    else
	echo "$*" 1>&2
	echo "$*" >>$LOG
    fi
    unset oflag C N NTC T
}

select_timezone()
{
    if [ $1 != GMT ]; then
	ls /usr/share/lib/zoneinfo/$1 >/tmp/tzlist.$$
	LINES=`/usr/bin/awk 'END {print NR}' /tmp/tzlist.$$`
	/usr/bin/awk '{printf "%3d. %s\n", NR, $0}' /tmp/tzlist.$$ | pr -t -4
	/usr/bin/awk '{print NR ".", $0}' /tmp/tzlist.$$ >/tmp/tzlist
	printf "\n\tplease select (1-$LINES): "
	read LINE
	COUNTRY=`/usr/bin/grep "^${LINE}. " /tmp/tzlist | /usr/bin/awk '{print $2}'`
	CHECK=`/usr/bin/grep -c "^${LINE}. " /tmp/tzlist`
	if [ $CHECK -eq 0 ]; then
	    print "\n\t\tInvalid country - please try again!"
	    echo
	else
	    print "\n\tSelected: ${1}/${COUNTRY}"
	    return $COUNTRY
	    rm /tmp/tzlist*
	    break
	fi
    else
	print "\n\tSelected: ${1}"
	CHECK=1
    fi
    rm /tmp/tzlist*
}

setup_log()
{
    [ -n "$LOG" ] && return
    exec 4>/dev/console
    exec 1>>$1
    exec 2>>$1
    LOG=$1
}

# currently on SPARC, the zpool command dumps core, if trying
# to create zpools on a EFI labeld disk - therefore EFI labels
# aren't supported on SPARC atm.
validate_disk()
{
    print "\n\tChecking disk(s)..."
    case $1 in
	[0-9],[0-9])
	    dchk1=`echo $1 | awk -F, '{print $1}'`
	    dchk2=`echo $1 | awk -F, '{print $2}'`
	    disk=`/usr/bin/awk '/ '"$dchk1"'\./ {print $2}' /tmp/dsk.log`
	    mirror=`/usr/bin/awk '/ '"$dchk2"'\./ {print $2}' /tmp/dsk.log`
	;;
	    [0-9]) disk=`/usr/bin/awk '/ '"$1"'\./ {print $2}' /tmp/dsk.log`
	;;
	    c[0-9]*t[0-9]*d[0-9]*s[0-7]|c[0-9]*d[0-9]*s[0-7])
	    # check if the user provided a valid zpool
	    print "\n\tChecking zpool on: $1 ..."
	    IPORT=`/usr/sbin/zpool list | grep -c NAME`
	    if [ "$IPORT" -eq 0 ]; then
		printf "\tDid you forgot to import your prepared zpool ? - exit 0"
		sleep 5; exit 0
	    fi
	    for pool in `zpool list | sed \
		-e "/^NAME/d" \
		-e "/no pools/d" | awk '{print $1}'`
	    do
		ISIN=`/usr/sbin/zpool status $pool | grep -c ${1}`
		if [ "$ISIN" -eq 1 ]; then
		    pool_name=$pool
		    break
		fi
	    done
	    if [ "$ISIN" -eq 1 -a -n "$pool_name" ]; then
		instslice=$1
		defpart=no
		nozpool=yes
		disk=`echo $1 | sed -e "s@s.*@@g"`
	    else
		if [ $ARCH = sparc ]; then
		    printf "\n\tdisk-slice: $1 has not a valid ZPOOL - exit." ; sleep 5; exit 0
		fi
	    fi
	;;
	    *) print "\n\tonly two disks allowed for mirroring... - exit." ; sleep 5 ; exit 0
	;;
    esac

    print "\nThe following disk has been selected for installation:" >>$LOG
    nawk  '/'"$disk "'/,/ / {gsub("^[ ,0-9]*. ",""); print $0}' /tmp/dsk.log >>$LOG
    if [ -n "$mirror" ]; then
	nawk  '/'"$mirror "'/,/ / {gsub("^[ ,0-9]*. ",""); print $0}' /tmp/dsk.log >>$LOG
    fi

    if [ "$numlen" -eq 1 ]; then
	validate_zpool $disk
    else
	validate_zpool $disk
	#validate_zpool $mirror mirror
    fi

    if [ "$ARCH" = "sparc" ]; then
	for echk in $disk $mirror
	do
	    /usr/sbin/prtvtoc /dev/rdsk/${echk}s2 >/dev/null2>/dev/null
	    if [ $? -ge 1 ]; then
		print "\tdisk: $echk uses EFI disklabel - not supported on SPARC."
		printf "\tDo you want to convert the EFI label into SMI label for: $echk (default=Y/n?): "
		if yesno; then
		    printf "\tConverting..."
		    echo "l\n0\n\nq\n" | /usr/sbin/format -e $echk >/dev/null 2>/dev/null
		    smiconv=true
    		    print "\nDisk: ${echk} has been converted from EFI to SMI label." >>$LOG
		    echo " done"
		fi
	    fi
	done
	if [ "$smiconv" = "true" ]; then
	    print "\n\tYou just have converted EFI into SMI labels,"
	    printf "\tplease use the format program in a SHELL to partition your disk(s)"
	    sleep 10; exit 0
	fi
	zslice=0
	disk_okay=okay
    else
	printf "\n\n\tSearching for Operating Systems"
	# UEFI search... vtoc-tag 22 is Linux swap if needed here.
	/usr/sbin/prtvtoc -h /dev/rdsk/${disk}s2 2>/dev/null >/tmp/vtoc.$$
	efis=`awk '$2==12 {print $1}' /tmp/vtoc.$$`; printf "."
	msr=`awk '$2==19 {print "yes"}' /tmp/vtoc.$$`
	ntfs=`awk '$2==20 {print "yes"}' /tmp/vtoc.$$`
	resv=`awk '$2==11 {print "yes"}' /tmp/vtoc.$$`; printf "."
	# MBR/BIOS search...
	# create initial fdisk-table.
	/sbin/fdisk -W -v /dev/rdsk/${disk}p0 | /usr/bin/egrep -v "^\*|^$" >/tmp/fdisk_table.$$
	# check for Solaris2 partition.
	illu=`awk '$1==0191{print "yes"}' /tmp/fdisk_table.$$`
	# used partitions
	numuse=`/usr/bin/egrep -c -v "^  0|^  191" /tmp/fdisk_table.$$`
	printf "."
	if [ -n "$efis" -a "$msr" = "yes" -a "$ntfs" = "yes" ]; then
	    print "\nNo forign Operating Systems found on above disk drive." >>$LOG
	    echo " done"
	    efiboot $disk
	elif [ "$EFIBOOT" -eq 0 -a "$numuse" -eq 0 ]; then
	    echo " done"
	    do_zpool
	    disk_okay=okay
	else
	    if [ "$EFIBOOT" -eq 1 ]; then
		printf "\n\n\tDo you want to setup a Multiboot Environment (default=N/y?): "
		if noyes; then
		    if [ "$reinstall" = true ]; then
			zlice=0
			do_zpool
			disk_okay=okay
		    else
			if [ -n "$efifs" -a -n "$ntfs" ]; then
			    printf "\tLast chance really destroy exsiting operating system (default=Y/n?): "
			    if yesno; then
				do_zpool
				disk_okay=okay
			    else
				printf "\t\t\tokay - no change has been done - exit."
				exit 0
			    fi
			else
			    do_zpool
			    disk_okay=okay
			fi
		    fi
		else
		    efiboot $disk
		fi
	    else
		echo " done"
		biosboot $disk
	    fi
	fi
    fi
}

# Function to validate hostname
validate_hostname() {
    Hostname="$1"

    # Length check
    if [ ${#Hostname} -lt 1 ] || [ ${#Hostname} -gt 253 ]; then
	echo "Invalid: Hostname must be between 1 and 253 characters."
	return 1
    fi

    # Allowed characters check
    echo "$Hostname" | /usr/bin/grep -qE '^[a-zA-Z0-9][a-zA-Z0-9-]*[a-zA-Z0-9]$|^[a-zA-Z0-9]$'
    if [ $? -ne 0 ]; then
	echo "Invalid: Hostname must contain only letters, numbers,"
	echo "or hyphens, and cannot start or end with a hyphen."
	return 1
    fi
    return 0
}

validate_netmask()
{
    Subnet="$1"
    if [ `echo ${Subnet} | egrep "[0-9,a-f]{8}"` >/dev/null 2>&1 ]; then
	# we got a hex mask like ffffff00
	for i in 0 2 4 6; do
	    ones=$(printf "obytes=2;%d\n" "0x${Subnet:${i}:2}" | bc | tr -d 0)
	    mask=$(( ${mask} + ${#ones} )) # add the amount of remaining ones to bits
	done
    fi
    if [ `echo ${Subnet} | egrep "[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+"` >/dev/null 2>&1 ]; then
	# we have a decimal netmask
	echo ${Subnet} | tr '.' ' ' | read decnm[1] decnm[2] decnm[3] decnm[4]
	for i in 1 2 3 4; do
	    if [ ${decnm[${i}]} -gt 255 ]; then
		echo "Invalid decimal netmask, must be x.x.x.x with x >0 and <=255"
		sleep 5
	    fi
	    # convert decimal to binary and delete zeros
	    ones=$( echo "obytes=2;${decnm[${i}]}" | bc | tr -d 0)
	    # add the amount of remaining ones to bits
	    mask=$(( ${mask} + ${#ones} ))
	done
    fi
    if [ ${mask} -eq 0 ]; then
	if [ ${Subnet##/} -le 32 -a ${Subnet##/} -gt 0 ]; then
	#     we got a CIDR prefix, either /xx or xx
	    mask=${Subnet##/}
	else
	    echo "Invalid CIDR, must be >1 and <=32"
	    sleep 5
	fi
    fi
}

# Function to validate zpool for installation
validate_zpool()
{
    # rpool name
    if [ -z "$pool_name" ]; then
	log +tc "Checking system for pools..."
	# check if pool has been cretated in advance.
	if [ `zpool list | grep -c "^NAME"` -eq "1" ]; then
	    # check for more than one created/imported pool.
	    if [ `zpool list | sed -e "/^NAME/d" | wc -l | awk '{print $1}'` -gt "1" ]; then
		log +nt "Only one initial zpool supported - next try"; sleep 5; exit 0
	    fi
	    pool_name=`zpool list | sed -e "/^NAME/d" | awk '{print $1}'`
	else
	    /usr/sbin/zpool import >/tmp/ztemp.log 2>/dev/null
	    if [ -s /tmp/ztemp.log ]; then
		# this must be a re-install.
		sed -e "/^ config:/,+1d" /tmp/ztemp.log >/tmp/zpool.log
		for id in `awk '/id:/ {print $2}' /tmp/zpool.log`
		do
		    P2L=`grep -B 1 $id /tmp/zpool.log | awk '/pool:/ {print $2}'`
		    grep -B 1 $id /tmp/zpool.log | head -1 >/tmp/${P2L}_${id}_zpool.log
		    nawk '/'"$id"'/,/^$/ {print $0}' /tmp/zpool.log >>/tmp/${P2L}_${id}_zpool.log
		    ISIN=`grep -c ${1} /tmp/${P2L}_${id}_zpool.log`
		    if [ "$ISIN" -eq 1 ]; then
			F2S=/tmp/${P2L}_${id}_zpool.log
			break
		    fi
		done
	    else
		log " none found, pool name for install: rpool"
		pool_name=rpool
		if [ -z "$defpart" ]; then
		    defpart=yes
		fi
	    fi
	fi
	if [ -n "$F2S" ]; then
	    if [ "$2" = mirror ]; then
		log +nt"tYour disk mirror choice: $1 is used by zpool:"
	    else
		log +nt "Your disk choice: $1 is used by zpool:"
	    fi
	    /usr/bin/cat $F2S
	    /usr/bin/cat $F2S >>$LOG
	    NLINE=`awk '{line = $NR} END {print line}' $F2S`
	    if [ -z "$NLINE" ]; then
		echo
	    fi
	    log +t "if you continue, all data on it will be destroyed !!!"
	    log +tc "please confirm (default=Y/n?): "
	    if yesno; then
		pool_name=rpool
		reinstall=true
		log +tc "Destroying existing zpool: $pool_name..."
		zpool import $pool_name
		zpool destroy $pool_name
		log " done"
		[ -z "$defpart" ] && defpart=yes
	    else
		if [ -n "$SDSK" ]; then
		    pool_name=$P2L defpart=yes
		else
		    exit 0
		fi
	    fi
	else
	    pool_name=rpool
	fi
	rm -f /tmp/*_zpool.log*
    fi
}

yesno()
{
    while true
    do
	read response
	case "$response" in
	    [Yy]|[Yy][Ee][Ss]|"")
		[ -z "$nolog" ] && echo y >>$LOG
		return 0
	    ;;
	    [Nn]|[Nn][Oo])
		echo n >>$LOG
		return 1
	    ;;
	    *)
		printf "\t\tPlease answer yes (y) or no (n): "
	    ;;
	esac
    done
}

### main ###
# ensure that we are root.
ROOT=`/usr/bin/id | /usr/bin/sed -e "s@^uid=@@g" -e "s@(.*@@g"`
if [ "$ROOT" != "0" ]; then
    echo "\n\tSorry - only root is allowed to run this script - exit.\n"
    exit 1
fi

# make sure we only run if we are called on a installing system.
OK2RUN=`/usr/bin/svcs -a | /usr/bin/awk '/system\/install-setup/ {print $1}'`
if [ "$OK2RUN" != "online" ]; then
    echo "\nThis script should only be run on a system, which is to be installed - exit.\n"
    exit 1
fi

# set some global variables.
eval `uname -ap | \
    awk '{
	print "KARCH="$5;
	print "ARCH="$6;
	print "PLATFORM="$7;
	}'`
NSVC=false
LOG=/tmp/install_log
TLOG=/tmp/migration_log
# set inital netmask variables.
mask=0
ones=0

# Restore our modified dhcpagent defaults file.
if [ -f /etc/default/dhcpagent.orig ]; then
    mv -f /etc/default/dhcpagent.orig /etc/default/dhcpagent
fi
if [ -f /lib/svc/method/net-svc.orig ]; then
    mv -f /lib/svc/method/net-svc.orig /lib/svc/method/net-svc
fi
if [ -f /lib/svc/share/smf_include.sh.orig ]; then
    mv -f /lib/svc/share/smf_include.sh.orig /lib/svc/share/smf_include.sh
fi

# it writeable, curious just needs to be done on SPARC
rm /etc/inet/hosts
cp -p /mnt/misc/etc/inet/hosts /etc/inet/hosts

setup_log $LOG

# OpenIndina:
distribution=`/usr/bin/awk '/powered by illumos/ {print $1}' /etc/release`
# OmniOS:
[ -z "$distribution" ] && distribution=`/usr/bin/awk 'NR==1 {print $1}' /etc/release`

# initialize /tmp/install.log
[ -f $LOG ] && rm $LOG
print "\n\t*** $distribution Install Log ***\n" >$LOG
print "Kernel architecture: $KARCH" >>$LOG
print "Architecture: ${ARCH}" >>$LOG
/usr/sbin/prtdiag -v | awk '$2 ~ /Configuration/ || $2 ~ /size/ || $1 ~ /BIOS/' >>$LOG
if [ "$ARCH" = i386 ]; then
    /usr/sbin/prtconf | grep "^Memory" >>$LOG
    printf "This is a x86_64 system using " >>$LOG
    EFIBOOT=`/usr/sbin/prtconf -v 2>/dev/null | /usr/bin/grep -c efi-systab`
    if [ "$EFIBOOT" -eq 1 ]; then
	PART="GPT/UEFI"
    else
	PART="MBR/BIOS"
    fi
    print "$PART based partitioning schema." >>$LOG
fi

# disk selection - start
CLEANSSCREEN=0
disk_okay=no
while [ $disk_okay != "okay" ]; do
    if [ ! -f /tmp/devfsadm.run ]; then
	printf "\n\tBuilding initial device tree - please wait..."
	/usr/sbin/devfsadm 2>/dev/null >/dev/null
	touch /tmp/devfsadm.run
	echo " done"
    fi
    clear
    echo
    print "\t\t  Welcome to $distribution text-installer\n"
    print "Thanks for choosing to install ${distribution}! This installer enables you to"
    print "install the $distribution Operating System (OS) on x86 or SPARC systems.\n"
    print "The installation takes place using ZFS filesystems on slice 0 of the disk you"
    print "will be choosing below. If this is not what you want, please use <q> down below"
    print "and <3> in the top-level menu to exit to a SHELL. Use the <format> and <zpool>"
    print "commands to prepare the disk-slice you want to install on. For Multiboot,"
    print "slice or mirrored install options, consult the README.multiboot in /jack."
    print "NOTE: From time to time it can happen, that you see a dump of the device tree,"
    print "in this case, just press ^C to to restart all over from the top menu.\n"
    printf "\tSeeking disks on system..."
    format >/tmp/dsk.log <<EOF 2>/dev/null
EOF
    if [ `/usr/bin/grep -c "^AVAIL" /tmp/dsk.log` -eq 1 ]; then
	print " done\n"
	/usr/bin/awk '/ [0-9]*\. /' /tmp/dsk.log
	printf "\n\tChoose disk or slice e.g. c0t0d0s0 to install on: "
	read num
	numlen=`expr "$num" : '.* *'`
	case $num in
	    [0-9]*) validate_disk $num
	    ;;
	    [0-9]*,[0-9]*) validate_disk $num
	    ;;
	    c[0-9]*d[0-9]*s[0-7]) validate_disk $num
	    ;;
	    c[0-9]*t[0-9]*d[0-9]*s[0-7]) validate_disk $num
	    ;;
	    q) echo "\nBye, bye...\n"; sleep 1; exit 0
	    ;;
	    *) echo "\n\tNo such disk/slice: $num - next try"; sleep 5
	esac
    else
	print " No disks found!"
	printf "\n\tPlease check disk configuration - and come back..."
	sleep 5
	exit 0
    fi
done
rm /tmp/dsk.log
dlenght=`expr "$disk" : '.* *'`
# disk selection - end

rm -f /tmp/zpool.log
[ -f /tmp/ztemp.log ] && rm /tmp/ztemp.log
plenght=`expr "$pool_name" : '.* *'`
[ -z "$instslice" ] && instslice="${disk}s${zslice}"
if [ -n "$mirror" ]; then
    mirrslice=`echo $instslice | sed -e "s@$disk@$mirror@g"`
    mlenght=`expr "$mirrslice" : '.* *'`
fi
slenght=`expr "$instslice" : '.* *'`

# checking for DHCP, created by svc:/network/physical:nwam
if [ -f /etc/resolv.conf ]; then
    # try to find out which nic was used for DHCP.
    nic=`/sbin/ipadm show-addr | /usr/bin/awk -F/ '/dhcp/ && /ok/ {print $1}'`
    if [ -n "$nic" ]; then
	numnic=`echo $nic | wc -w | awk '{print $1}'`
	if [ "$numnic" -eq "1" ]; then
	    DHCP=true
	    log +nt "Network configuration acquired by dhcp."
	    [ -f /tmp/vars.sh ] && rm -f /tmp/vars.sh
	    for var in Subnet Timeserv Router DNSserv Hostname DNSdmain Yiaddr \
			Rootpath Broadcst NISdmain NISservs NTPservs
	    do
		dget=`/sbin/dhcpinfo $var`
		if [ -n "$dget" ]; then
		    pastem=`echo $dget | wc -w | awk '{print $1}'`
		    if [ "$pastem" -gt 1 ]; then
			echo $dget >/tmp/merge
			dget=`cat /tmp/merge`
			set $var="$dget"
			rm -f /tmp/merge
		    fi
		    echo "$var=\"$dget\"" >>/tmp/vars.sh
		fi
	    done
	    . /tmp/vars.sh
	    cat /tmp/vars.sh >>$LOG
	    rm -f /tmp/vars.sh
	    [ -n "$Subnet" ] && validate_netmask $Subnet
	    # get IPv6 address probably for static use later.
	    IPv6ip=`ipadm show-addr | \
		/usr/bin/nawk '/'"$nic"'/ && $2 ~ /addrconf/ {sub("%.*",""); print $4}'`
	else
	    print "\tIt seems there is more than one network interface under DHCP control,"
	    printf "\tplease use just one DHCP network interface for installation! exit. "
	    /bin/sleep 20; exit 0
	fi
    fi
else
    DHCP=false
    nic=none
fi

if [ -n "$NISdmain" -a -n "$Hostname" -a "$nic" != none ]; then
    log +t "NIS domain: $NISdmain found using DHCP."
    log +tc "Do you want to join: $Hostname into above domain (default=Y/n?): "
    if yesno; then
	log +tc "Setting up NIS client: $Hostname in $NISdmain"
	NSVC=true
	/usr/bin/uname -S $Hostname
	echo "$NISdmain" >/etc/defaultdomain
	/usr/bin/domainname $NISdmain
	log +c "."
	/usr/sbin/ypinit -c "$NISservs" 2>/dev/null
	log +c "."
	sleep 1
	/usr/sbin/svcadm enable network/nis/client:default
	sleep 2
	# fix ypmatch hostname resolution.
	rm /var/yp/binding/${NISdmain}/ypservers
	for nserv in $NISservs
	do
	    /usr/bin/ypmatch $nserv hosts.byaddr >> /etc/hosts
	    grep "^${nserv}" /etc/hosts | \
	    awk '{print $2}' >> /var/yp/binding/${NISdmain}/ypservers
	    csrv=`grep "^${nserv}" /etc/hosts | awk '{print $2}'`
	    newserv="$csrv $newserv"
	    log +c "."
	    /usr/bin/ypmatch $nserv hosts.byaddr >> /tmp/hosts.add
	done
	/usr/sbin/svcadm restart network/nis/client:default >/dev/null 2>&1
	sleep 1
	NISservs=`echo $newserv | sed -e "s@ \\$@@g"`
	TNUM=`ypcat timezone.byname | awk 'END{print NR}'`
	[ -n "$TNUM" ] && TIMEZONE=`ypcat timezone.byname | awk '{print $1}'`
	log +c "."
	log " done"
	nfs4dom=`echo $NISdmain | cut -d\. -f2-`
	log +t "NFSv4 Domain, derived from NISdomain has been set to: $nfs4dom"
	if [ -n "$Router" ]; then
	    router=`/usr/bin/ypmatch $Router hosts.byaddr | awk '{print $2}'`
	fi
	ntype=static DHCP=false
    else
	log +t "Not joining above NIS domain: ${NISdmain}\n"
	unset NISdmain NISservs
    fi
fi

if [ "$nic" != none -a -z "$NISdmain" -a -z "$NISservs" ]; then
    if [ -z "$Hostname" ]; then
	while true
	do
	    printf "\tHostname: "
	    read Hostname
	    if validate_hostname "$Hostname"; then
		/usr/bin/hostname "$Hostname"
		log "Hostname set to: $Hostname"
		break
	    else
		echo "Hostname not set due to validation failure."
	    fi
	done
    fi
    log +t "The following network configuration has been detected on: ${nic}\n"
    log +t "  Hostname: $Hostname"
    log +t "    Router: $Router"
    log +t "IP-Address: $Yiaddr"
    log +t "Subnetmask: ${Subnet}"
    if [ -n "$DNSdmain" ]; then
	NSVC=true
	log +t "DNS domain: $DNSdmain"
	/usr/bin/grep "^nameserver" /etc/resolv.conf >>$LOG
    fi
    log +nt "System: $Hostname will be configured using Dynamic Host"
    log +t "Configuration Protocol, if you accept the default."
    log +tc "Do you want to use DHCP for network configuration (default=Y/n?): "
    if yesno; then
	log +t "The network configuration will be done using DHCP."
	net=okay ntype=dynamic
    else
	log +t "Do you want to use above information to setup: $nic with"
	log +tc "static IP configuration, to omit DHCP control on it (default=Y/n?): "
	if yesno; then
	    net=okay ntype=static nic=$nic DHCP=false
	    log +t "Static network configuration choosen using the parameters provided by DHCP."
	else
	    net=none nic=none ntype=static DHCP=false
	fi
    fi
fi

if [ "$nic" = none ]; then
    net=none
    # cleanup from a previous run.
    [ -f /etc/resolv.conf ] && rm -f /etc/resolv.conf
    echo "\n\tWhich network interface do want to use ?\n"
    while [ $net != "okay" ]; do
	tnics=`/sbin/dladm show-phys | /usr/bin/awk '/Ethernet/ {printf $1 "|"}'`
	/sbin/dladm show-phys | /usr/bin/awk '/Ethernet/ {print "\t\t\t\t\t    " $1}'
	printf "\n\t\t\t       Your choice: "
	read nic
	case $nic in
	    $tnics) net=okay
		print "\nNetwork interface choosen for installation: $nic" >>$LOG
	    ;;
	    *) echo "\n\t\t\tInvalid network-interface - please try again!\n"
	    ;;
	esac
    done
    DHCP=false
    printf "\t\t\t\t  Hostname: "
    read Hostname
    if validate_hostname "$Hostname"; then
	/usr/bin/hostname "$Hostname"
	print "Hostname: $Hostname" >>$LOG
    else
	echo "Hostname not set due to validation failure."
    fi
    printf "\t\t\t\tIP-Address: "
    read Yiaddr
    print "IP-Address: $Yiaddr" >>$LOG
    while true
    do
	printf "\tEnter netmask decimal, hex or CIDR: "
	read Subnet
	validate_netmask $Subnet
	if [ $mask -ne 0 ]; then
	    print "Netmask: $mask" >>$LOG
	    break
	fi
    done
    printf "\t\t\t\t    Router: "
    read Router
    print "Default-Router: $Router" >>$LOG
    printf "\t\t\t\tDNS Domain: "
    read DNSdmain
    if [ `echo "$DNSdmain" | grep -c "\."` -ge "1" ]; then
	echo "domain  ${DNSdmain}" >/etc/resolv.conf
	echo "search $DNSdmain" >>/etc/resolv.conf
	print "DNS Domain: $DNSdmain" >>$LOG
    else
	unset DNSdmain
    fi
    printf "DNS server IPs max. 3, seperated by spaces: "
    read DNSserv
    if [ `echo "$DNSserv" | grep -c "\."` -ge "1" ]; then
	for srv in $DNSserv
	do
	    echo "nameserver  $srv" >> /etc/resolv.conf
	done
	print "DNS Servers:" >>$LOG
	cat /etc/resolv.conf >>$LOG
    else
	unset DNSserv
    fi
    if [ -z "$DNSdmain" ] && [ -z "$DNSserv" ]; then
	log +nt "Disable Domain Name Services no information given !\n"
    else
	log +tc "Configuring network with given values"
	svcadm disable svc:/network/physical:nwam 2>&1 >/dev/null
	svcadm enable svc:/network/physical:default
	sleep 1
	log +c "."
	/sbin/ipadm create-if $nic
	/sbin/ipadm create-addr -T addrconf ${nic}/v4link
	log +c "."
	IPv6ip=`ipadm show-addr | \
		/usr/bin/nawk '/'"$nic"'/ && $2 ~ /addrconf/ {sub("%.*",""); print $4}'`
	log +c "."
	/sbin/ipadm create-addr -T static -a local=${Yiaddr}/${mask} ${nic}/v4
	log +c "."
	/sbin/route -p add default $Router 2>&1 >/dev/null
	cp -p /etc/nsswitch.dns /etc/nsswitch.conf
	svcadm restart svc:/system/name-service-cache:default
	log " done"
	log +tc "Testing network with given values"
	publisher=`/usr/bin/pkg publisher | nawk '/openindiana.org/ {
		gsub("^.*://","");
		gsub("/.*",""); print $0}'`
	log +c "."
	dnsresp=`/usr/bin/nslookup $publisher | awk '/^Name:/ {print $2}'`
	log +c "."
	if [ "$publisher" = "$dnsresp" ]; then
	    log ". done"
	    NSVC=true
	else
	    log ". failed, disable software selection!"
	    NSVC=false
	fi
    fi
    ntype=static
    DHCP=false
    echo
fi

# set lenght of network interface variable.
nlenght=`expr "$nic" : '.* *'`

while true
do
    printf "\n\tDo you want to convert the root account to be a role (default=N/y?): "
    if noyes; then
	print "Superuser root NOT converted to be role" >>$LOG
	DO_RBAC=no; break
    else
	print "Superuser root converted to be role" >>$LOG
	DO_RBAC=yes; break
    fi
done

while true
do
    printf "\tDo you want to enable legacy network services (default=N/y?): "
    if noyes; then
	print "Legacy network services NOT enabled" >>$LOG
	RLOGIN=no; break
    else
	print "Legacy network services enabled" >>$LOG
	RLOGIN=yes; break
    fi
done

# and NFSv4 stuff
nfs4=unknown
if [ -z "$nfs4dom" ]; then
    printf "\tNFSv4 Domain Name  <RETURN> for none: "
    while [ $nfs4 != "okay" -o $nfs4 != "none" ]; do
        read nfs4dom
        case $nfs4dom in
	    [a-z,A-Z,0-9,.]*) print "NFSv4 Domain Name set to: $nfs4dom" >>$LOG
		nfs4=okay ; break
	    ;;
	    *) nfs4=none ; print "No NFSv4 Domain Name has been set" >>$LOG
		nfs4=none ; break
	    ;;
	esac
    done
fi

HEX_DATA=`/sbin/dhcpinfo Vendor | tr -d '\n' | sed 's/0x//g'`
# try to get TIMEZONE via DHCP option 43, set to the usual TIMEZONE string.
if [ -z "$TIMEZONE" ]; then
    if [ -n "$HEX_DATA" ]; then
	TIMEZONE=$(extract_subopt "65")
    fi
fi

# if still no TIMEZONE we try the posix variant.
if [ -z "$TIMEZONE" ]; then
    TIMEZONE=$(extract_subopt "64")
fi

# timezone/date handling.
if [ -z "$TIMEZONE" ]; then
    print "\n\tSelect the region that contains your time zone.\n"
    print "\tRegions"
    print "\t______________________________________________\n"
    while true
    do
	echo "UTC/GMT" >/tmp/myregion
	cut -d'	' -f2- /usr/share/lib/zoneinfo/tab/continent.tab | \
		/usr/bin/sed "/^#/d" >> /tmp/myregion
	/usr/bin/awk '{printf("\t%2d. ", NR); print $0}' /tmp/myregion
	rm /tmp/myregion
	printf "\n\tplease select: "
	read region
	case $region in
	    1) ZDIR=GMT; break ;;
	    2) ZDIR=Africa; break ;;
	    3) ZDIR=America; break ;;
	    4) ZDIR=Antarctica; break ;;
	    5) ZDIR=Arctic; break ;;
	    6) ZDIR=Asia; break ;;
	    7) ZDIR=Atlantic; break ;;
	    8) ZDIR=Australia; break ;;
	    9) ZDIR=Europe; break ;;
	    10) ZDIR=Indian; break ;;
	    11) ZDIR=Pacific; break ;;
	    *) echo "\n\t\tInvalid Region: $region - please try again!\n" ;;
	esac
    done

    if [ "$ZDIR" != "GMT" ]; then
	print "\n\tSelect the location that contains your time zone.\n"
	print "\tLocations"
	print "\t______________________________________________\n"
	CHECK=0
	while [ $CHECK -ne 1 ]; do
	    select_timezone $ZDIR
	done
    fi

    if [ "$ZDIR" = "GMT" ]; then
	TIMEZONE=GMT
    else
	TIMEZONE="${ZDIR}/${COUNTRY}"
    fi
fi
print "Selected TIMEZONE: $TIMEZONE" >>$LOG
export TZ=$TIMEZONE

if [ -n "$Timeserv" -o -n "$NTPservs" ]; then
    log +ntc "Syncronizing time using "
    if [ -z "$NTPservs" ]; then
	timeserv=`echo $Timeserv | awk '{print $1}'`
	if [ -n "$NISdmain" ]; then
	    timeserv=`/usr/bin/ypmatch $timeserv hosts.byaddr | awk '{print $2}'`
	fi
	cmd="/usr/bin/rdate"
	STYPE="RFC 868"
	SHOW="${STYPE}-"
    elif [ -z "$Timeserv" ]; then
	timeserv=`echo $NTPservs | awk '{print $1}'`
	if [ -n "$NISdmain" ]; then
	    timeserv=`/usr/bin/ypmatch $timeserv hosts.byaddr | awk '{print $2}'`
	cmd="/usr/sbin/ntpdate"
	STYPE="NTP"
	SHOW="${STYPE}-"
	fi
    else
	SHOW=""
    fi
    SHOW="${STYPE}${TFMT}"
    log "${SHOW}server: ${timeserv}..."
    $cmd $timeserv >/dev/null 2>/dev/null
    log +tc "System Time has been set to: "
    CURT=`/usr/bin/date`
    log "$CURT done"
else
    log +tc "Date/Time is: "
    date '+%m/%d/%y %c%H:%M:%S'
    date '+%m/%d/%y %c%H:%M:%S' >>$LOG
    log +tc "correct (default=Y/n?): "
    if ! yesno; then
	printf "\tSpecify new date/time - format: Month/Day/Hour/Minute: "
	while true
	do
	    read ntime
	    if [ `expr "$ntime" : '.* *'` -ne "8" ]; then
		printf "\n\twrong format single digits have to prefixed with '0' - try again!\n"
		printf "\n\tDate/Time: "
	    else
		date $ntime >/dev/null
		log +tc "Date/Time manually set to: " >>$LOG
		date '+%m/%d/%y %c%H:%M:%S'
		date '+%m/%d/%y %c%H:%M:%S' >>$LOG
		break
	    fi
	done
    fi
fi

LAYOUT=US-English
TYPE=`/usr/bin/kbd -l | /usr/bin/awk -F'[= ]' '{if ($1 == "layout") print $2}'`
if [ -n $TYPE ]; then
    LAYOUT=`/usr/bin/awk -v ntyp=$TYPE -F= '{if ($2 == ntyp) print $1}' /usr/share/lib/keytables/type_6/kbd_layouts`
fi

# try to find default router.
if [ -z $Router ]; then
    Router=`netstat -rn | /usr/bin/awk '/default/ {print $2}' | sort -u`
fi

# to be able to use the passwd command we first have to remove /etc/passwd
# and /etc/group then copy them over from /mnt/misc/etc
rm /etc/passwd /etc/group
cp -p /mnt/misc/etc/passwd /etc/passwd
cp -p /mnt/misc/etc/group /etc/group
# get the old timestamp of /etc/shadow to at least ensure that passwd has been used,
# however since root is using passwd command we can't realy tell if a passwd has been
# set, because passwd also accepts just return :-( for sure this needs improvement.
STAMP=`/usr/bin/awk -F: '/^root/ {print $3}' /mnt/misc/etc/shadow`
print "\n\tSet System root password and define a user account for yourself.\n"
print "\tSystem Root Password:\n"
while true
do
    /usr/bin/passwd
    SETPW=`/usr/bin/awk -F: '/^root/ {print $3}' /etc/shadow`
    if [ $SETPW -gt $STAMP ]; then
	RLINE=`/usr/bin/awk '/^root/' /etc/shadow`
	break
    fi
done

# 1000 is the default for UID/GID since ages.
uid=1000
gid=1000
print "\n\tCreate a local user account."
printf "\tYour real name:\t"
read rname
while true
do
    printf "\t      Username:\t"
    read lname
    ulenght=`expr "$lname" : '.* *'`
    printf "    UID (default=${uid}): "
    read uid
    case $uid in
	[0-9]*)
	    if [ `expr "$uid" : '.* *'` -lt "5" ]; then
		print "\n\twrong UID range given - must be at least 5 digits.\n"
	    else
		break
	    fi
	;;
	    [a-z,A-Z]*) print "\n\twrong UID format given - can only contian digits.\n"
	;;
	*) uid=1000
	    break
	;;
    esac
done

while true
do
    printf "    GID (default=${gid}): "
    read gid
    case $gid in
	[0-9]*)
	    if [ `expr "$gid" : '.* *'` -lt "4" ]; then
		print "\n\twrong GID range given - must be at least 4 digits.\n"
	    else
		break
	    fi
	;;
	[a-z,A-Z]*) print "\n\twrong UID format given - can only contian digits.\n"
	;;
	*) gid=1000
	    break
	;;
    esac
done
echo "users::${gid}:" >> /etc/group

print "\n\tWhich SHELL do you want for account: $lname ?\n"
while true
do
    print "\t\t1. /usr/bin/ksh"
    print "\t\t2. /usr/bin/csh"
    print "\t\t3. /usr/bin/tcsh"
    print "\t\t4. /usr/bin/bash\n"
    printf "\tYour choice: 1,2,3 or 4 : "
    read resp
    case $resp in
	1) SH=/usr/bin/ksh ; break ;;
	2) SH=/usr/bin/csh ; break ;;
	3) SH=/usr/bin/tcsh ; break ;;
	""|4) SH=/usr/bin/bash ; break ;;
	*) printf "\n\twrong SHELL choosen - try again!"; sleep 5; print "\n" ;;
    esac
done

printf "\n${lname}'s "
echo "${lname}:x:${uid}:${gid}:${rname}:/tmp/${lname}:${SH}" >>/etc/passwd
echo "${lname}::6445::::::" >>/etc/shadow
/usr/bin/passwd -r files $lname
UHASH=`/usr/bin/awk -F: '/'"$lname"'/ {print $2}' /etc/shadow`
UHOME=/export/home
if [ -n "$NISdmain" -a -n "$NISservs" ]; then
    aihome=`ypmatch -k $lname auto.home | awk -F: '{print $1}'`
    if [ "$lname" = "$aihome" ]; then
	nolog=1
	printf "\n\tHOME for: $lname on NFS by default (default=n/Y?): "
	if yesno; then
	    UHOME=/home
	fi
	unset nolog
    fi
fi

# check if we have at least one nic up and running and a working name service.
if [ `/sbin/dladm show-phys | grep -c up` -ge "1" -a "$NSVC" = "true" ]; then
    while true
    do
	print "\n\tChoose your final installation type:\n"
	print "\t1. Server		2. SunRay - includes Mate Desktop"
	print "\t3. Developer		4. Mate Desktop System"
	print "\t5. Devolper + Mate 	6. SunRay + Developer"
	print "\t7. Server + Developer + Mate Desktop System"
	print "\t8. Server + Developer	9. All Combinations\n"
	print "\tq. don't install additional software right now\n"
	printf "\tPlease make your choice (1,2,3,4,5,6,7,8,9 or q): "
	read choice
	case $choice in
	    1) addsw=server_install
		dev=no
		UPD="Server System"
		break
	    ;;
	    2) addsw="mate_install consolidation/sunray/sunray-essential"
		dev=no
		UPD="SunRay Server"
		break
	    ;;
	    3) addsw="metapackages/build-essential"
		dev=yes
		UPD="Developer System"
		break
	    ;;
	    4) addsw=mate_install
		dev=no
		UPD="Mate Desktop System"
		break
	    ;;
	    5) addsw="metapackages/build-essential mate_install"
		dev=yes
		UPD="Mate Desktop System and Developer Support"
		break
	    ;;
	    6) addsw="consolidation/sunray/sunray-essential mate_install metapackages/build-essential"
		dev=yes
		UPD="SunRay Server + Developer System"
		break
	    ;;
	    7) addsw="server_install metapackages/build-essential mate_install"
		dev=yes
		UPD="Server, Developer and Mate Desktop System"
		break
	    ;;
	    8) addsw="server_install metapackages/build-essential"
		dev=yes
		UPD="Server + Developer Support"
		break
	    ;;
	    9) addsw="server_install consolidation/sunray/sunray-essential metapackages/build-essential mate_install"
		dev=yes
		UPD="Server, SunRay Server, Developer and Mate Desktop System"
		break
	    ;;
	    q) break
	    ;;
	    *) echo "\n\t\twrong input - try again.\n"
	    ;;
    esac
  done
fi

print "\nThe follwoing system configuration has been acquired:" >>$LOG
print "====================================================" >>$LOG
print "\n\tPlease review the settings below before installing. If not okay, just"
print "\tpress ^C to start all over from the previous menu, using option <1>\n"
log "           Hostname: $Hostname with IP-Address: $Yiaddr"
log "      RBAC for root: $DO_RBAC"
log "  Network Interface: $nic configuration type: $ntype"
if [ -n "$router" ]; then
    log "     Default Router: $router"
elif [ -n "$Router" ]; then
    log "     Default Router: $Router"
fi
if [ "$STYPE" = NTP ]; then
    log "      NTP server(s): $timeserv"
elif [ -n "$NTPservs" ]; then
    log "      NTP server(s): $NTPservs"
fi
print "   User Account for: $rname"
print "         Login Name: $lname - UID=$uid GID=${gid}(users)"
log "           Keyboard: $LAYOUT"
log +c "  Current Date/Tine: "
date
date >>$LOG
log "           TIMEZONE: $TIMEZONE"
if [ -n "$nfs4dom" ]; then
    log "  NFSv4 Domain Name: $nfs4dom"
fi
if [ -n "$NISdmain" ]; then
    log "	 NIS Domain: $NISdmain"
    log "      NIS server(s): $NISservs"
fi
if [ -s /etc/resolv.conf ]; then
    log +c "	 DNS Domain: "
    /usr/bin/awk '/domain/ {print $2}' /etc/resolv.conf
    /usr/bin/awk '/domain/ {print $2}' /etc/resolv.conf >>$LOG
    log +c "      DNS server(s): "
    for srv in `/usr/bin/awk '/nameserver/ {print $2}' /etc/resolv.conf`
    do
	log +c "$srv "
    done
    log ""
fi
if [ -n "$mirror" ]; then
    log " Disk configuration: mirrored disks on: ${disk} & ${mirror}"
else
    log " Disk configuration: single disk on: ${disk}"
fi

if [ -n "$addsw" ]; then
  log "  Installation Type: $UPD"
fi 

printf "\n\tSettings okay? just hit <RETURN> to start the installation: "
read go

# remove the temporary /sbin/resize binary, needed for installation.
[ -x /sbin/resize ] && rm -f /sbin/resize

# get size of slice 2 and relabel slice 0 if default partitioning has been used.
if [ $defpart = yes ]; then
    # constuct a label script
    if [ -n "$mirror" ]; then
	len=`expr $mlenght + $dlenght`
	printf "\nLabeling disks: ${disk}/${mirror}"
	dneed=`expr 52 - $len`
    else
	printf "\nLabeling disk: ${disk}"
	dneed=`expr 52 - $dlenght`
    fi
    BLKS=`/usr/sbin/prtvtoc /dev/rdsk/$instslice | /usr/bin/awk '$1 ~ /2/ {print $5}'`
    echo "p\n0\nroot\nwm" > /tmp/label_script
    echo "0" >>/tmp/label_script
    echo "${BLKS}b" >>/tmp/label_script
    echo "1\nunassigned\nwm\n0\n0" >>/tmp/label_script
    echo "3\nunassigned\nwm\n0\n0" >>/tmp/label_script
    echo "4\nunassigned\nwm\n0\n0" >>/tmp/label_script
    echo "5\nunassigned\nwm\n0\n0" >>/tmp/label_script
    echo "6\nunassigned\nwm\n0\n0" >>/tmp/label_script
    echo "7\nunassigned\nwm\n0\n0" >>/tmp/label_script
    echo "q\nl" >>/tmp/label_script
    format -s -d ${disk} -f /tmp/label_script
    rm /tmp/label_script
    if [ -n "$mirror" ]; then
	/usr/sbin/prtvtoc -h /dev/rdsk/${disk}s2 | fmthard -s - /dev/rdsk/${mirror}s2 1>/dev/null
    fi
    while [ "$dneed" -ne "0" ]; do printf "."; dneed=`expr ${dneed} - 1`; done
    echo " done"
else
    echo
fi

printf "\nInstallation for $distribution starting at: " >>$LOG
date +%Y/%m/%d-%H:%M:%S >>$LOG

ALTROOT=/a
/usr/bin/mkdir -p ${ALTROOT}
len=`expr $plenght + $slenght`
if [ -z "$nozpool" ]; then
    if [ -n "$mirror" ]; then
	len=`expr $len + $mlenght`
	log +c "Creating: ${pool_name} on: ${instslice}/${mirrslice}"
	/usr/sbin/zpool labelclear -f ${instslice} 2>/dev/null
	/usr/sbin/zpool labelclear -f ${mirrslice} 2>/dev/null
	/usr/sbin/zpool create -f -o failmode=continue ${pool_name} mirror ${instslice} ${mirrslice}
	dneed=`expr 51 - ${len}`
    else
	dneed=`expr 47 - ${len}`
	log +c "Creating: ${pool_name} on disk: ${instslice}"
	/usr/sbin/zpool labelclear -f ${instslice} 2>/dev/null
	/usr/sbin/zpool create -f -o failmode=continue ${pool_name} ${instslice}
    fi
    while [ "$dneed" -ne "0" ]; do printf "."; dneed=`expr ${dneed} - 1`; done
    log " done"
fi

log +c "Creating needed ZFS filesystems on zpool: ${pool_name}"
/usr/sbin/zfs create -o mountpoint=legacy ${pool_name}/ROOT
/usr/sbin/zfs create -o mountpoint=${ALTROOT} ${pool_name}/ROOT/openindiana
/usr/sbin/zfs create -o mountpoint=${ALTROOT}/export ${pool_name}/export
printf "."
/usr/sbin/zfs create -o mountpoint=${ALTROOT}/export/home ${pool_name}/export/home
# calculate size of dump/swap volumes.
MEM=`/usr/sbin/prtconf | awk '/^Memory/ {print $3}'`
if [ "$MEM" -lt "2050" ]; then
    VSZ=2048
elif [ "$MEM" -lt "4100" ]; then
    VSZ=4096
elif [ "$MEM" -lt "8200" ]; then
    VSZ=8192
else
    VSZ=16384
fi
log +c "."
/usr/sbin/zfs create -V ${VSZ}m ${pool_name}/dump
log +c "."
/usr/sbin/zfs create -V ${VSZ}m -b 8k ${pool_name}/swap
log +c "."
/usr/sbin/zfs set org.opensolaris.libbe:uuid=`/usr/bin/uuidgen` ${pool_name}/ROOT/openindiana
log +c "."
/usr/sbin/zfs create -o mountpoint=${ALTROOT}/var ${pool_name}/ROOT/openindiana/var
log +c "."
/usr/sbin/zpool set bootfs="${pool_name}/ROOT/openindiana" ${pool_name}
dneed=`expr 19 - ${plenght}`
while [ "$dneed" -ne "0" ]; do printf "."; dneed=`expr ${dneed} - 1`; done
log " done"

# transfer the OS now.
log +c "Populating the /etc directory.................."
START=`truss -t time date 2>&1 1>/dev/null | /usr/bin/sed -n "1s/.*= //p"`
cd /mnt/misc
/usr/bin/find etc -depth -print | cpio -pdmu ${ALTROOT} 2>/dev/null
rm /etc/svc/repository.db
cp /lib/svc/seed/global.db ${ALTROOT}/etc/svc/repository.db
(cd /etc/svc; ln -s ${ALTROOT}/etc/svc/repository.db repository.db)
echo $Hostname > ${ALTROOT}/etc/nodename
sed -e "s@localhost.*@$Hostname ${Hostname}.local localhost loghost@g" ${ALTROOT}/etc/inet/hosts >\
/tmp/ihosts
mv /tmp/ihosts ${ALTROOT}/etc/inet/hosts
(cd ${ALTROOT}/etc/svc/profile; ln -s inetd_generic.xml inetd_services.xml)

# set TIMEZONE.
/usr/bin/sed -i s:PST8PDT:$TIMEZONE: ${ALTROOT}/etc/default/init

# fix /etc/system
/usr/bin/sed -e "/^set zfs/d" ${ALTROOT}/etc/system >/tmp/fix_etc_system
/usr/bin/mv /tmp/fix_etc_system ${ALTROOT}/etc/system
/usr/bin/chgrp sys ${ALTROOT}/etc/system

# add swap space
echo "/dev/zvol/dsk/${pool_name}/swap\t-\t-\tswap\t-\tno\t-" >>${ALTROOT}/etc/vfstab

END=`truss -t time date 2>&1 1>/dev/null | /usr/bin/sed -n "1s/.*= //p"`
SEC=`expr $END - $START`
while [ `expr "$SEC" : '.* *'` -ne "3" ]
do
    SEC=" "$SEC
done
log " done: ${SEC} seconds needed."

log +c "Populating the root filesystem.........."
START=`truss -t time date 2>&1 1>/dev/null | /usr/bin/sed -n "1s/.*= //p"`
cd /
for i in boot kernel lib platform root sbin usr
do
    /usr/bin/find $i -print -depth | cpio -pdm ${ALTROOT} 2>/dev/null
    log +c "."
done
END=`truss -t time date 2>&1 1>/dev/null | /usr/bin/sed -n "1s/.*= //p"`
SEC=`expr $END - $START`
while [ `expr "$SEC" : '.* *'` -ne "3" ]
do
    SEC=" "$SEC
done
log " done: ${SEC} seconds needed."

log +c "Populating the /var filesystem................."
START=`truss -t time date 2>&1 1>/dev/null | /usr/bin/sed -n "1s/.*= //p"`
cd /mnt/misc/var
/usr/bin/find . -depth -print | cpio -pdm ${ALTROOT}/var 2>/dev/null
# needed if run a second time, for debugging.
[ -f /var/pkg/lock ] && /bin/rm /var/pkg/lock
[ -f /var/pkg/modified ] && /bin/rm /var/pkg/modified
END=`truss -t time date 2>&1 1>/dev/null | /usr/bin/sed -n "1s/.*= //p"`
SEC=`expr $END - $START`
while [ `expr "$SEC" : '.* *'` -ne "3" ]
do
    SEC=" "$SEC
done
log " done: ${SEC} seconds needed."

if [ -f /.usr.tar.xz ]; then
    log +c "Populating the /usr directory..................."
    START=`truss -t time date 2>&1 1>/dev/null | /usr/bin/sed -n "1s/.*= //p"`
    cd /
    xzcat .usr.tar.xz | ( cd ${ALTROOT}/usr; tar xpf - )
    END=`truss -t time date 2>&1 1>/dev/null | /usr/bin/sed -n "1s/.*= //p"`
    SEC=`expr $END - $START`
    while [ `expr "$SEC" : '.* *'` -ne "3" ]
    do
	SEC=" "$SEC
    done
    log "done: ${SEC} seconds needed."
fi

log +c "Adding extra directories."
cd ${ALTROOT}
/usr/bin/ln -s ./usr/bin .
log +c "."
/usr/bin/mkdir -m 1777 tmp
log +c "."
/usr/bin/mkdir -p system/contract system/object system/boot proc mnt dev devices/pseudo
log +c "."
/usr/bin/mkdir -p dev/fd dev/rmt dev/swap dev/dsk dev/rdsk dev/net dev/ipnet
log +c "."
/usr/bin/mkdir -p dev/sad dev/pts dev/term dev/vt dev/zcons
log +c "."
/usr/bin/chgrp -R sys dev devices mnt
log +c "."
/usr/bin/chmod 0555 system system/* proc
log +c "."
cd dev
/usr/bin/ln -s ./fd/2 stderr
/usr/bin/ln -s ./fd/1 stdout
log +c "."
/usr/bin/ln -s ./fd/0 stdin
log +c "."
/usr/bin/ln -s ../devices/pseudo/dld@0:ctl dld
log +c "."
cd /
log +c "............"
log " done"

log +c "Setting default keyboard-layout................"
sed -e "s@US-English@$LAYOUT@g" /usr/share/install/sc_template.xml >\
    ${ALTROOT}/etc/svc/profile/sc_profile.xml
(cd ${ALTROOT}/etc/svc/profile; ln -s sc_profile.xml site.xml)
echo '#!/usr/bin/ksh\n' >${ALTROOT}/etc/sysding.conf
log " done"

SVCCFG_REPOSITORY=${ALTROOT}/etc/svc/repository.db
export SVCCFG_REPOSITORY
if [ $DHCP = true ]; then
    log +c "Configure $nic for DHCP use"
    dneed=`expr 22 - $nlenght`
    if [ -n "$Router" ]; then
	echo "setup_route default $Router" >>${ALTROOT}/etc/sysding.conf
    fi
    log +c "."
    /usr/sbin/svccfg -s network/physical:default setprop general/enabled=false
    log +c "."
    /usr/sbin/svccfg -s network/physical:nwam setprop general/enabled=true
    while [ "$dneed" -ne "0" ]; do printf "."; dneed=`expr ${dneed} - 1`; done
else
    log +c "Configure Network: $nic"
    dneed=`expr 25 - $nlenght`
    echo "setup_interface $nic v4 ${Yiaddr}/${mask}" >>${ALTROOT}/etc/sysding.conf
    log +c "."
    echo "/usr/sbin/svcadm disable network/physical:nwam" >>${ALTROOT}/etc/sysding.conf
    echo "/usr/sbin/svcadm enable network/physical:default" >>${ALTROOT}/etc/sysding.conf
    if [ -n "$IPv6ip" ]; then
	echo "setup_interface $nic v6 ${IPv6ip}/10" >>${ALTROOT}/etc/sysding.conf
    fi
    log +c "."
    echo "setup_route default $Router" >>${ALTROOT}/etc/sysding.conf
    log +c "."
    while [ "$dneed" -ne "0" ]; do log +c "."; dneed=`expr ${dneed} - 1`; done
fi
if [ "$NSVC" = "false" ]; then
    (cd ${ALTROOT}/etc/svc/profile; ln -s ns_files.xml name_service.xml)
fi
(cd ${ALTROOT}/etc/svc/profile; ln -s generic_limited_net.xml generic.xml)
log " done"

if [ $RLOGIN = yes ]; then
    log +c "Configure Legacy Network Services"
    dneed=14
    echo "/usr/sbin/svcadm enable network/shell:default" >>${ALTROOT}/etc/sysding.conf
    echo "/usr/sbin/svcadm enable network/login:rlogin" >>${ALTROOT}/etc/sysding.conf
    while [ "$dneed" -ne "0" ]; do log +c "."; dneed=`expr ${dneed} - 1`; done
    log " done"
fi

if [ -n "$NTPservs" ]; then
    log +c "Configure Network Time Protocol"
    dneed=15
    sed -e "/^server/,+1d" ${ALTROOT}/etc/inet/ntp.client >${ALTROOT}/etc/inet/ntp.conf
    log +c "."
    for serv in $NTPservs
    do
	echo "server $serv iburst" >>${ALTROOT}/etc/inet/ntp.conf
    done
    echo "/usr/sbin/svcadm enable network/ntp:default" >>${ALTROOT}/etc/sysding.conf
    while [ "$dneed" -ne "0" ]; do log +c "."; dneed=`expr ${dneed} - 1`; done
    log " done"
fi

if [ -n "$NISdmain" -a -n "$NISservs" ]; then
    log +c "Configure Network Information Service (NIS)"
    cp -p /etc/defaultdomain ${ALTROOT}/etc
    (cd ${ALTROOT}/etc/svc/profile;
    [ -h name_service.xml ] && rm name_service.xml;
    ln -s ns_nis.xml name_service.xml)
    (cd ${ALTROOT}/etc; cp -p nsswitch.nis nsswitch.conf)
    log +c "..."
    mkdir -p ${ALTROOT}/var/yp/binding/$NISdmain
    cp -p /var/yp/binding/${NISdmain}/ypservers ${ALTROOT}/var/yp/binding/$NISdmain
    log +c "."
    cat /tmp/hosts.add >> ${ALTROOT}/etc/inet/hosts
    /usr/sbin/svccfg import /lib/svc/manifest/network/nis/client.xml
    /usr/sbin/svccfg -s network/nis/client:default setprop general/enabled=true
    /usr/sbin/svcadm refresh network/nis/client:default
    log " done"
fi

log +c "Configure Dump Device.................."
WD=`pwd`
log +c "."
dumpmajor=`awk '/^dump / {print $2}' ${ALTROOT}/etc/name_to_major`
log +c "."
cd ${ALTROOT}/devices/pseudo
log +c "."
/usr/sbin/mknod dump@0:dump c $dumpmajor 0
log +c "."
cd ../../dev
log +c "."
ln -s ../devices/pseudo/dump@0:dump dump
log +c "."
/usr/sbin/dumpadm -r ${ALTROOT} >/dev/null
log +c "."
/usr/bin/sed \
	-e "s@/crash.*@/crash/${Hostname}@g" \
	-e "s@swap@/dev/zvol/dsk/${pool_name}/dump@g" ${ALTROOT}/etc/dumpadm.conf \
	> /tmp/dumpadm.conf
log +c "."
mv /tmp/dumpadm.conf ${ALTROOT}/etc
log " done"

if [ "$nfs4dom" ]; then
    log +c "Configure NFSv4 Domain Name...................."
    echo "setup_nfs4domain $nfs4dom" >>${ALTROOT}/etc/sysding.conf
    log " done"
fi

if [ -z "$NISdmain" -a -z "$NISservs" -a -n "$DNSdmain" ]; then
    log +c "Configure DNS name-service..."
    # add the search directive to resolv.conf.
    echo "domain  $DNSdmain" >${ALTROOT}/etc/resolv.conf
    log +c "..."
    echo "search $DNSdmain" >>${ALTROOT}/etc/resolv.conf
    log +c "..."
    for srv in $DNSserv
    do
	echo "nameserver  $srv" >>${ALTROOT}/etc/resolv.conf
    done
    chown netadm:netadm ${ALTROOT}/etc/resolv.conf
    (cd ${ALTROOT}/etc/svc/profile;
    [ -h name_service.xml ] && rm name_service.xml;
    ln -s ns_dns.xml name_service.xml)
    (cd ${ALTROOT}/etc; cp -p nsswitch.dns nsswitch.conf)
    log "...........  done"
fi

log +c "Configure users/groups......"
echo $RLINE >${ALTROOT}/etc/shadow
log +c "....."
/usr/bin/sed "/^root/d" /mnt/misc/etc/shadow >>${ALTROOT}/etc/shadow
log +c "....."
echo "${lname}:${UHASH}:${SETPW}::::::" >>${ALTROOT}/etc/shadow
log +c "."
/usr/bin/chmod 0400 ${ALTROOT}/etc/shadow
log +c "....."
/usr/bin/chgrp sys ${ALTROOT}/etc/shadow
echo "${lname}:x:${uid}:${gid}:${rname}:${UHOME}/${lname}:${SH}" >>${ALTROOT}/etc/passwd
log +c "..."
log " done"

printf "Add login: $lname to RBAC"
echo "${lname}::::profiles=Primary Administrator;roles=root" >>${ALTROOT}/etc/user_attr
dneed=`expr 28 - ${ulenght}`
while [ "$dneed" -ne "0" ]; do printf "."; dneed=`expr ${dneed} - 1`; done
print " done"

if [ "$DO_RBAC" = yes ]; then
    log +c "Converting root account to role................"
    /usr/bin/sed "/^root/d" ${ALTROOT}/etc/user_attr >/tmp/user_attr.$$
    printf "root::::min_label=admin_low;lock_after_retries=no;" >>/tmp/user_attr.$$
    printf "auths=solaris.*,solaris.grant;audit_flags=lo\\" >>/tmp/user_attr.$$
    echo ":no;profiles=All;clearance=admin_high;type=role" >>/tmp/user_attr.$$
    mv /tmp/user_attr.$$ ${ALTROOT}/etc/user_attr
    /usr/bin/chgrp sys ${ALTROOT}/etc/user_attr
    log " done"
fi

printf "Add local HOME for user: ${lname}"
/usr/bin/mkdir ${ALTROOT}/export/home/${lname}
cp -p /etc/skel/.bashrc ${ALTROOT}/export/home/$lname
(cd ${ALTROOT}/export/home; /usr/bin/chown -R ${lname}:${gid} $lname)
/usr/sbin/zfs umount ${pool_name}/export/home
/usr/sbin/zfs umount ${pool_name}/export
dneed=`expr 22 - ${ulenght}`
while [ "$dneed" -ne "0" ]; do printf "."; dneed=`expr ${dneed} - 1`; done
print " done"

log +c "Add group: users with GID ${gid}"
echo "users::${gid}:" >>${ALTROOT}/etc/group
log "................. done"

# packages we don't want on installed systems.
DPKG="system/install/media/internal system/install/text-install system/install"
# needs to be checked if really a gui-install
#DPKG="$DPKG system/install/gui-install"

# gather some unwanted packages/services on a platform basis.
if [ "$KARCH" = sun4u ]; then
  DPKG="$DPKG system/kernel/cpu/sun4v"
fi
if [ "$ARCH" = sparc ]; then
    if [ "$PLATFORM" != "SUNW,Sun-Fire-880" ] || [ "$PLATFORM" != "SUNW,Sun-Fire-V890" ]; then
	DPKG="$DPKG system/kernel/dynamic-reconfiguration/sun-fire-880"
    fi
    if [ "$PLATFORM" != "SUNW,Sun-Fire-15000" ]; then
	DPKG="$DPKG system/kernel/dynamic-reconfiguration/sun-fire-15000"
    fi
    if [ "$PLATFORM" != "SUNW,Sun-Fire-15000" ] || [ "$PLATFORM" != "SUNW,SPARC-Enterprise" ]; then
	# Note: FMD still depends on:
	# system/domain-service-processor-protocol/sparc-enterprise
	# on systems on which it isn't needed - needs to be fixed in illumos-gate.
	DPKG="$DPKG service/key-management/sun-fire-15000"
	DPKG="$DPKG system/domain-configuration/sparc-enterprise"
    fi
    # configure IPsec/routing on SPARC M-Class servers for ppp communication.
    if [ "$PLATFORM" = "SUNW,SPARC-Enterprise" ]; then
	XSCF=`/usr/platform/${PLATFORM}/sbin/prtdscp | awk '/^SP/ {print $3}'`
	DOMADDR=`/usr/platform/${PLATFORM}/sbin/prtdscp | awk '/^Domain/ {print $3}'`
	if [ -n "$XSCF" ] && [ -n "$DOMADDR" ]; then
	    echo "{laddr $DOMADDR raddr $XSCF} ipsec" >${ALTROOT}/etc/inet/ipsecinit.conf
	    echo "    {encr_algs aes encr_auth_algs sha256 sa shared}" >>${ALTROOT}/etc/inet/ipsecinit.conf
	fi
	# this is to silence these console messages:
	# in.routed[474]: route 10.1.1.1/32 --> 10.1.1.2 nexthop is not directly connected
	if [ -z "$Router" ]; then
	    echo "$Router" >${ALTROOT}/etc/defaultrouter
	    /usr/sbin/svccfg -s network/routing/route:default setprop restarter/state=disabled
	fi
    fi
fi

log +c "Uninstalling unwanted packages................."
/usr/bin/pkg -R ${ALTROOT} uninstall -q --no-backup-be $DPKG
log " done"

if [ -n "$addsw" ]; then
    START=`truss -t time date 2>&1 1>/dev/null | /usr/bin/sed -n "1s/.*= //p"`
    log +c "Checking userland-incorporation "
    ULNOW=`pkg info consolidation/userland/userland-incorporation | \
	/usr/bin/nawk '/FMRI:/ {
	    sub("^.*-[0-9]*.0.0.",""); sub(":.*","");print $1}'`
    (cd /var; mv pkg pkg.orig; ln -s /a/var/pkg pkg)
    /usr/bin/pkg refresh -q --full &
    pkg_pid=$!
    dial $pkg_pid
    ULREP=`pkg list -H -n --no-refresh consolidation/userland/userland-incorporation@latest 2>/dev/null | \
	/usr/bin/nawk '{gsub("^.*[0-9,0-9,0-9,0-9].0.0.",""); print $1}'`
    echo "\b\b\c"
    END=`truss -t time date 2>&1 1>/dev/null | /usr/bin/sed -n "1s/.*= //p"`
    SEC=`expr $END - $START`
    while [ `expr "$SEC" : '.* *'` -ne "3" ]; do
	SEC=" "$SEC
    done
    if [ "$ULNOW" -lt "$ULREP" ]; then
	log "................. done: $SEC seconds needed."
	log "\nUpdating installed base system:"
	log "-------------------------------"
	/usr/bin/pkg -R ${ALTROOT} update | tee -a $LOG
	echo "" >> $LOG
    else
	log "................. done"
    fi
    log +c "Creating package dependencies"
    for i in $addsw
    do
	/usr/bin/pkg contents -rm $i | nawk '/^depend/ {
	    gsub("^.*fmri=","");
	    gsub("@.*","");
	    gsub("pkg:/","");
	    print $1
	}' >>/tmp/pkgs1.depend
	log +c "."
    done
    echo "consolidation/l10n/l10n-incorporation" >>/tmp/pkgs1.depend
    # this dirty hack is need because of realy bad designed package names.
    ISIN=`grep -c "network/dns/bind" /tmp/pkgs1.depend`
    [ "$ISIN" -ge "1" ] && echo "network/dns/bind" >>/tmp/pkgs.add
    ISIN=`grep -c "gnome/gnome-keyring" /tmp/pkgs1.depend`
    [ "$ISIN" -ge "1" ] && echo "gnome/gnome-keyring" >>/tmp/pkgs.add
    ISIN=`grep -c "network/ssh" /tmp/pkgs1.depend`
    [ "$ISIN" -ge "1" ] && echo "network/ssh" >>/tmp/pkgs.add
    ISIN=`grep -c "network/ftp" /tmp/pkgs1.depend`
    [ "$ISIN" -ge "1" ] && echo "network/ftp" >>/tmp/pkgs.add
    ISIN=`grep -c "network/telnet" /tmp/pkgs1.depend`
    [ "$ISIN" -ge "1" ] && echo "network/telnet" >>/tmp/pkgs.add
    log +c "."
    sed \
	-e "/_install/d" \
	-e "/gnome\/gnome-keyring/d" \
	-e "/network\/dns\/bind/d" \
	-e "/network\/ftp/d" \
	-e "/network\/telnet/d" \
	-e "/network\/ssh/d" \
	-e "/^system\/osnet\/locale\//d" \
	-e "/^locale\//d" /tmp/pkgs1.depend >/tmp/pkgs2.depend
    log +c "."
    if [ -f /tmp/pkgs.add ]; then
	for i in `cat /tmp/pkgs.add`
	do
	    echo "pkg:///${i}" >>/tmp/pkgs2.depend
	    log +c "."
	done
    fi
    ISIN=`grep -c "^locale/" /tmp/pkgs1.depend`
    if [ "$ISIN" -ge "1" ]; then
	pkg list -n | egrep "^system/osnet/locale/|^locale/" >/tmp/locale_list
	egrep "^system/osnet/locale/|^locale/" /tmp/pkgs1.depend | sort -u >/tmp/locs.depend
	for loc in `cat /tmp/locs.depend`
	do
	    grep "^${loc} " /tmp/locale_list | awk '{print $1 "@" $2}' >>/tmp/pkgs2.depend
	done
	rm /tmp/locale_list /tmp/locs.depend
    fi
    /usr/bin/sort -u /tmp/pkgs2.depend >/tmp/pkgs3.depend
    log +c "."
    for i in `cat /tmp/pkgs3.depend`
    do
	printf "$i " >>/tmp/pkgs.depend
    done
    echo "" >>/tmp/pkgs.depend
    log ".......... done"
    log "Installing: ${UPD}..."
    print "\nInstalling Software Collection: ${UPD}...\n" >>$LOG
    /usr/bin/pkg -R ${ALTROOT} install --no-backup-be `cat /tmp/pkgs.depend` | tee -a $LOG
    rm /tmp/pkgs.depend /tmp/pkgs1.depend /tmp/pkgs2.depend /tmp/pkgs3.depend /tmp/pkgs.add
fi

if [ "$dev" = "yes" ]; then
    print "\nAdding OI-Software Maintainer Role for: $lname"
    dneed=11
    sed "/^${lname}/d" ${ALTROOT}/etc/user_attr >/tmp/user_attr.$$
    awk '/^'"$lname"'/ {
	print $0 ";type=normal;profiles=Software Installation"}' ${ALTROOT}/etc/user_attr \
    >>/tmp/user_attr.$$
    mv /tmp/user_attr.$$ ${ALTROOT}/etc/user_attr
    chgrp sys ${ALTROOT}/etc/user_attr
fi

# set default boot device.
if [ "$ARCH" = "sparc" ]; then
    IBOOT="/usr/sbin/installboot"
    log +c "\nSetting OBP default boot-device"
    # first find out if we have installed to a SCSI controller on a SUNW,Ultra-5_10.
    case $PLATFORM in
	SUNW,Ultra-5_10|SUNW,Sun-Blade-100)
	IDE=`ls -al /dev/dsk/$instslice | grep -c sd`
	if [ "$IDE" = 0 ]; then
	    OBPDEV=`ls -al /dev/dsk/$instslice | \
	    /usr/bin/nawk '{
		sub("^.*../../devices","");
		sub("dad","disk");
		print $0}'`
	else
	    OBPDEV=`ls -al /dev/dsk/$instslice | \
	    /usr/bin/nawk '{
		sub("^.*../../devices","");
		print $0}'`
	fi
	;;
	*)
	    OBPDEV=`ls -al /dev/dsk/$instslice | \
	    /usr/bin/nawk '{
		sub("^.*../../devices","");
		sub("ssd","disk");
		sub("sd","disk");
		print $0}'`
	;;
    esac
    if [ -n "$mirror" ]; then
	case $PLATFORM in
	    SUNW,Ultra-5_10|SUNW,Sun-Blade-100)
	    if [ "$IDE" = 0 ]; then
		MIRRDEV=`ls -al /dev/dsk/$mirrslice | \
		/usr/bin/nawk '{
		    sub("^.*../../devices","");
		    sub("dad","disk");
		    print $0}'`
	    else
		MIRRDEV=`ls -al /dev/dsk/$mirrslice | \
		/usr/bin/nawk '{
		    sub("^.*../../devices","");
		    print $0}'`
	    fi
	    ;;
	    *)
		MIRRDEV=`ls -al /dev/dsk/$mirrslice | \
		/usr/bin/nawk '{
		    sub("^.*../../devices","");
		    sub("ssd","disk");
		    sub("sd","disk");
		    print $0}'`
	    ;;
	esac
	/usr/sbin/eeprom "boot-device=$OBPDEV $MIRRDEV"
    else
	/usr/sbin/eeprom "boot-device=$OBPDEV"
    fi
    dneed=16
    while [ "$dneed" -ne "0" ]; do printf "."; dneed=`expr ${dneed} - 1`; done
    log " done"
    BLK="/usr/platform/`uname -i`/lib/fs/zfs/bootblk"
    if [ -n "$mirror" ]; then
	log +c "Install Bootblocks on: ${instslice}/${mirrslice}..."
	ICMD="${IBOOT} -f -F zfs ${BLK} /dev/rdsk/${instslice}"
	ICMD="${ICMD}; ${IBOOT} -f -F zfs ${BLK} /dev/rdsk/${mirrslice}"
    else
	log +c "Installing Bootblock on: ${instslice}..."
	ICMD="${IBOOT} -f -F zfs ${BLK} /dev/rdsk/${instslice}"
    fi
    $ICMD
    log " done"
else
    if [ -f /tmp/menu.lst ]; then
	log +nc "Finish Multiboot Boot Setup..."
	[ ! -d /rpool/boot ] && mkdir -p /rpool/boot
	mv /tmp/menu.lst /rpool/boot
	log " done"
    fi
    log +nc "Installing Bootblock on: ${instslice}..."
    /usr/sbin/bootadm install-bootloader -fM -P $pool_name
    log " done"
fi

if [ "$needlin" = "yes" ]; then
    # move the illumos boot-loaders aside, to be managed by grub.
    log +nc "Finish Multiboot Boot Setup..."
    [ ! -d /tmp/pcfs ] && mkdir /tmp/pcfs
    mount -F pcfs /dev/dsk/${disk}s${efis} /tmp/pcfs
    [ ! -d /tmp/pcfs/EFI/${distribution}/Boot ] && mkdir -p /tmp/pcfs/EFI/${distribution}/Boot
    if [ -x /tmp/pcfs/EFI/Boot/bootx64.efi ]; then
	# make sure we can overwrite bootfiles.
	if [ -x /tmp/pcfs/EFI/${distribution}/Boot/bootx64.efi ]; then
	    chmod 777 /tmp/pcfs/EFI/${distribution}/Boot/bootx64.efi
	fi
	mv /tmp/pcfs/EFI/Boot/bootx64.efi /tmp/pcfs/EFI/${distribution}/Boot
    fi
    if [ -x /tmp/pcfs/EFI/Boot/bootia32.efi ]; then
	# make sure we can overwrite bootfiles.
	if [ -x /tmp/pcfs/EFI/${distribution}/Boot/bootia32.efi ]; then
	    chmod 777 /tmp/pcfs/EFI/${distribution}/Boot/bootia32.efi
	fi
	mv /tmp/pcfs/EFI/Boot/bootia32.efi /tmp/pcfs/EFI/${distribution}/Boot
    fi
    rm -rf /tmp/pcfs/EFI/Boot
    log " done"
    log +nc "Adding $distribution Boot-Manager plugin for Linux..."
    grub_cfg
    log " done"
    log "You have to run: /boot/efi/EFI/${distribution}/add_grub,"
    log "after the Linux installation has been done, to activate"
    log "$distribution in your grub configuration!\n"
    umount /tmp/pcfs
fi

log +nc "Configuring devices..."
${ALTROOT}/usr/sbin/devfsadm -r ${ALTROOT}
touch ${ALTROOT}/reconfigure
log " done"

/sbin/bootadm update-archive -R ${ALTROOT} -F cpio

log "Copying installation logs... done"
printf "\nInstallation done: " >>$LOG
date >>$LOG
if [ ! -d ${ALTROOT}/var/sadm/system/logs ]; then
    mkdir -p ${ALTROOT}/var/sadm/system/logs
    mv $LOG ${ALTROOT}/var/sadm/system/logs
    if [ -f /tmp/grub.cfg ]; then
	mv /tmp/grub.cfg ${ALTROOT}/var/sadm/system/logs
    fi
fi

# umount all ZFS filesystems, to be able to set new mountpoint properties.
printf "Umounting all ZFS filesystems..."
/usr/bin/sleep 5
/usr/sbin/zfs unmount -f -a
print " done"

# set final mountpoints for all ZFS filesystems.
printf "Set final mountpoint properties for all ZFS filesystems..."
/usr/sbin/zfs set canmount=noauto "${pool_name}/ROOT/openindiana"
/usr/sbin/zfs set mountpoint=/ "${pool_name}/ROOT/openindiana" 2>/dev/null
/usr/sbin/zfs set canmount=noauto "${pool_name}/ROOT/openindiana/var"
/usr/sbin/zfs set mountpoint=/var "${pool_name}/ROOT/openindiana/var" 2>/dev/null
/usr/sbin/zfs set org.opensolaris.libbe:policy=static "${pool_name}/ROOT/openindiana/var"
/usr/sbin/zfs inherit mountpoint "${pool_name}/ROOT/openindiana/var"
/usr/sbin/zfs inherit org.opensolaris.libbe:policy "${pool_name}/ROOT/openindiana/var"
/usr/sbin/zfs set mountpoint=/export "${pool_name}/export"
/usr/sbin/zfs set mountpoint=/export/home "${pool_name}/export/home"
print " done"

printf "\n\tInstallation done: "
date
print "\tRebooting now, please stand by...\n"
/usr/sbin/reboot
