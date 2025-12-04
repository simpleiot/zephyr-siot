#include "zephyr/sys/util.h"
#include <nvs.h>
#include <point.h>

#include <zephyr/net/http/server.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <sys/types.h>
#include <app_version.h>

#if defined(CONFIG_FAT_FILESYSTEM_ELM)
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <ff.h>
#endif

LOG_MODULE_REGISTER(siot, LOG_LEVEL_DBG);

// The following points will get persisted in NVS when the show up on
// zbus point_chan.
static const struct nvs_point nvs_pts[] = {
	{1, &point_def_boot_count, "0"}, {2, &point_def_description, "0"},
	{3, &point_def_staticip, "0"},   {4, &point_def_address, "0"},
	{5, &point_def_gateway, "0"},    {6, &point_def_netmask, "0"},
};

// ==================================================
// Network manager

static struct net_mgmt_event_callback mgmt_cb;

static void net_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			      struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected\n");
	}
}

// ==================================================
// SD Card

#if defined(CONFIG_FAT_FILESYSTEM_ELM)
static FATFS fat_fs;
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

static void sd_card_init(void)
{
	static const char *disk_pdrv = "SD";
	static const char *disk_mount_pt = "/SD:";
	int ret;

	LOG_INF("CLIFF: sd_card_init");

	// Raw disk i/o - check if disk is present
	do {
		uint64_t memory_size_mb;
		uint32_t block_count;
		uint32_t block_size;

		if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_CTRL_INIT, NULL) != 0) {
			LOG_ERR("Storage init ERROR!");
			break;
		}

		LOG_INF("SD card found");

		if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
			LOG_ERR("Unable to get sector count");
			break;
		}
		LOG_INF("Block count %u", block_count);

		if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
			LOG_ERR("Unable to get sector size");
			break;
		}
		LOG_INF("Sector size %u\n", block_size);

		memory_size_mb = (uint64_t)block_count * block_size;
		LOG_INF("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));

		if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_CTRL_DEINIT, NULL) != 0) {
			LOG_ERR("Storage deinit ERROR!");
			break;
		}
	} while (0);

	mp.mnt_point = disk_mount_pt;

	ret = fs_mount(&mp);
	if (ret != 0) {
		LOG_ERR("Failed to mount SD card (err: %d)", ret);
		return;
	}

	LOG_INF("SD card mounted at %s", mp.mnt_point);

	// List files in root directory
	struct fs_dir_t dirp;
	struct fs_dirent entry;

	fs_dir_t_init(&dirp);
	ret = fs_opendir(&dirp, mp.mnt_point);
	if (ret != 0) {
		LOG_ERR("Failed to open SD card directory (err: %d)", ret);
		return;
	}

	LOG_INF("Contents of %s:", mp.mnt_point);
	while (1) {
		ret = fs_readdir(&dirp, &entry);
		if (ret != 0 || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			LOG_INF("  [DIR]  %s", entry.name);
		} else {
			LOG_INF("  [FILE] %s (size: %u bytes)", entry.name, entry.size);
		}
	}

	fs_closedir(&dirp);
}
#endif

// ********************************
// main

int main(void)
{
	LOG_INF("SIOT MCU: %s %s", CONFIG_BOARD_TARGET, APP_VERSION_EXTENDED_STRING);

	// Fix GPIO0 value so WROVER clock out works
	if (strcmp(CONFIG_BOARD_TARGET, "esp32_poe_wrover/esp32/procpu") == 0) {
		LOG_DBG("Fix up GPIO0 on wrover");
		(*(volatile uint32_t *)0x3FF49044) = 0x1900;
	}

	nvs_init(nvs_pts, ARRAY_SIZE(nvs_pts));

#if defined(CONFIG_FAT_FILESYSTEM_ELM)
	sd_card_init();
#endif

	// In your initialization code:
	net_mgmt_init_event_callback(&mgmt_cb, net_event_handler, NET_EVENT_L4_CONNECTED);
	net_mgmt_add_event_callback(&mgmt_cb);
}
