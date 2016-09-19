foreign package android.os named com.livecode.wrapped.android.os

use java.lang.Object

class BatteryManager inherits Object
	constant ACTION_CHARGING as String is "android.os.action.CHARGING"
	constant ACTION_DISCHARGING as String is "android.os.action.DISCHARGING"
	constant BATTERY_HEALTH_COLD as int is 7
	constant BATTERY_HEALTH_DEAD as int is 4
	constant BATTERY_HEALTH_GOOD as int is 2
	constant BATTERY_HEALTH_OVERHEAT as int is 3
	constant BATTERY_HEALTH_OVER_VOLTAGE as int is 5
	constant BATTERY_HEALTH_UNKNOWN as int is 1
	constant BATTERY_HEALTH_UNSPECIFIED_FAILURE as int is 6
	constant BATTERY_PLUGGED_AC as int is 1
	constant BATTERY_PLUGGED_USB as int is 2
	constant BATTERY_PLUGGED_WIRELESS as int is 4
	constant BATTERY_PROPERTY_CAPACITY as int is 4
	constant BATTERY_PROPERTY_CHARGE_COUNTER as int is 1
	constant BATTERY_PROPERTY_CURRENT_AVERAGE as int is 3
	constant BATTERY_PROPERTY_CURRENT_NOW as int is 2
	constant BATTERY_PROPERTY_ENERGY_COUNTER as int is 5
	constant BATTERY_STATUS_CHARGING as int is 2
	constant BATTERY_STATUS_DISCHARGING as int is 3
	constant BATTERY_STATUS_FULL as int is 5
	constant BATTERY_STATUS_NOT_CHARGING as int is 4
	constant BATTERY_STATUS_UNKNOWN as int is 1
	constant EXTRA_HEALTH as String is "health"
	constant EXTRA_ICON_SMALL as String is "icon-small"
	constant EXTRA_LEVEL as String is "level"
	constant EXTRA_PLUGGED as String is "plugged"
	constant EXTRA_PRESENT as String is "present"
	constant EXTRA_SCALE as String is "scale"
	constant EXTRA_STATUS as String is "status"
	constant EXTRA_TECHNOLOGY as String is "technology"
	constant EXTRA_TEMPERATURE as String is "temperature"
	constant EXTRA_VOLTAGE as String is "voltage"
	
	method getIntProperty(id as int) returns int
	method getLongProperty(id as int) returns long
	method isCharging() returns boolean
end class

end package