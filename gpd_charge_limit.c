// SPDX-License-Identifier: GPL-2.0
/*
 * GPD Win Mini 2025 (G1617-02) battery charge limit
 *
 * The BIOS exposes a "Battery Full Charge Settings" option (60/70/80/90/100)
 * which is stored in the Setup variable at offset 0x82 and pushed to EC RAM
 * 0x1D by the Bds DXE driver on every boot, using the standard ACPI EC write
 * command. Nothing else in the firmware touches that byte: the DSDT declares
 * the surrounding fields but leaves 0x1D unnamed, and there is no SMM or ACPI
 * writer for it. That makes the register safe to drive from the OS, which is
 * what this module does.
 *
 * Behaviour of the IT5570 EC, determined by reverse engineering its firmware:
 *
 *  - charging stops once the capacity reaches the limit (for values 1..99),
 *    and resumes at limit - 10 (95 when the limit is >= 96);
 *  - at power-on, values below 60 are treated as 100, so only 60..100 is
 *    accepted here;
 *  - the EC appears to save changed values to its own flash, so callers
 *    should avoid writing this register frequently.
 *
 * Because the BIOS rewrites 0x1D on every boot, the BIOS menu setting acts as
 * the boot default and any value set through sysfs lasts until reboot.
 */

#include <linux/acpi.h>
#include <linux/dmi.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <acpi/battery.h>

#define GPD_EC_CHARGE_LIMIT	0x1d
#define GPD_CHARGE_LIMIT_MIN	60
#define GPD_CHARGE_LIMIT_MAX	100

static struct platform_device *gpd_pdev;

static const enum power_supply_property gpd_charge_limit_props[] = {
	POWER_SUPPLY_PROP_CHARGE_CONTROL_END_THRESHOLD,
};

static int gpd_charge_limit_get(struct power_supply *psy,
				const struct power_supply_ext *ext, void *data,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	u8 limit;
	int ret;

	if (psp != POWER_SUPPLY_PROP_CHARGE_CONTROL_END_THRESHOLD)
		return -EINVAL;

	ret = ec_read(GPD_EC_CHARGE_LIMIT, &limit);
	if (ret)
		return ret;

	/* The EC ignores 0 and anything >= 100: no limit */
	val->intval = (limit == 0 || limit > GPD_CHARGE_LIMIT_MAX) ?
		      GPD_CHARGE_LIMIT_MAX : limit;
	return 0;
}

static int gpd_charge_limit_set(struct power_supply *psy,
				const struct power_supply_ext *ext, void *data,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	if (psp != POWER_SUPPLY_PROP_CHARGE_CONTROL_END_THRESHOLD)
		return -EINVAL;

	if (val->intval < GPD_CHARGE_LIMIT_MIN || val->intval > GPD_CHARGE_LIMIT_MAX)
		return -EINVAL;

	return ec_write(GPD_EC_CHARGE_LIMIT, val->intval);
}

static int gpd_charge_limit_writeable(struct power_supply *psy,
				      const struct power_supply_ext *ext,
				      void *data, enum power_supply_property psp)
{
	return psp == POWER_SUPPLY_PROP_CHARGE_CONTROL_END_THRESHOLD;
}

static const struct power_supply_ext gpd_charge_limit_ext = {
	.name			= "gpd-charge-limit",
	.properties		= gpd_charge_limit_props,
	.num_properties		= ARRAY_SIZE(gpd_charge_limit_props),
	.get_property		= gpd_charge_limit_get,
	.set_property		= gpd_charge_limit_set,
	.property_is_writeable	= gpd_charge_limit_writeable,
};

static int gpd_add_battery(struct power_supply *psy, struct acpi_battery_hook *hook)
{
	if (strcmp(psy->desc->name, "BATT"))
		return 0;

	return power_supply_register_extension(psy, &gpd_charge_limit_ext,
					       &gpd_pdev->dev, NULL);
}

static int gpd_remove_battery(struct power_supply *psy, struct acpi_battery_hook *hook)
{
	if (strcmp(psy->desc->name, "BATT"))
		return 0;

	power_supply_unregister_extension(psy, &gpd_charge_limit_ext);
	return 0;
}

static struct acpi_battery_hook gpd_battery_hook = {
	.name		= "GPD charge limit",
	.add_battery	= gpd_add_battery,
	.remove_battery	= gpd_remove_battery,
};

static const struct dmi_system_id gpd_charge_limit_dmi[] = {
	{
		.ident = "GPD Win Mini 2025",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "GPD"),
			DMI_MATCH(DMI_PRODUCT_NAME, "G1617-02"),
		},
	},
	{ }
};
MODULE_DEVICE_TABLE(dmi, gpd_charge_limit_dmi);

static int __init gpd_charge_limit_init(void)
{
	if (!dmi_check_system(gpd_charge_limit_dmi))
		return -ENODEV;

	gpd_pdev = platform_device_register_simple("gpd-charge-limit",
						   PLATFORM_DEVID_NONE, NULL, 0);
	if (IS_ERR(gpd_pdev))
		return PTR_ERR(gpd_pdev);

	battery_hook_register(&gpd_battery_hook);
	return 0;
}

static void __exit gpd_charge_limit_exit(void)
{
	battery_hook_unregister(&gpd_battery_hook);
	platform_device_unregister(gpd_pdev);
}

module_init(gpd_charge_limit_init);
module_exit(gpd_charge_limit_exit);

MODULE_AUTHOR("Chengxin Wang <w@hxdl.org>");
MODULE_DESCRIPTION("GPD Win Mini 2025 battery charge limit");
MODULE_LICENSE("GPL");
