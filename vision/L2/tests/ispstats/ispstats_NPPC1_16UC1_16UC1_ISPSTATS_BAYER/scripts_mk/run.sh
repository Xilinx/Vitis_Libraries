qemuboot-tool \
	load BOOT-versal-2ve-2vm-vek385-sdt-seg.qemuboot.conf \
	remove image_link_name \
	remove image_name \
	merge  edf-linux-disk-image-amd-cortexa78-mali-common.rootfs.qemuboot.conf \
	remove staging_bindir_native \
	remove staging_dir_host \
	remove staging_dir_native \
	remove uninative_loader \
	> combined.qemuboot.conf
