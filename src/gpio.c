#include "dabai.h"

unsigned GetGpioVal(int gpio_num)
{
    char resulte[64], command[64];
    int reg_add, reg_val;
    int mod_v, div_v;

    div_v = gpio_num / 32;
    mod_v = gpio_num % 32;
    switch(div_v)
    {
      case 0:
        reg_add = GPIODATA_0;
        break;

      case 1:
        reg_add = GPIODATA_1;
        break;

      case 2:
        reg_add = GPIODATA_2;
        break;

      default:
        return -1;
        break;
    }
  	sprintf(command, "devmem 0x%08x", reg_add);
  	send_command(command, resulte, sizeof(resulte));
  	reg_val = strtoul(resulte + 2, NULL, 16);
    reg_val ^= (1 << mod_v);
    if(reg_val != 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

int SetGpioVal(int gpio_num, int val)
{
    char resulte[64], command[64];
    int reg_add, reg_val;
    int mod_v, div_v;

    div_v = gpio_num / 32;
    mod_v = gpio_num % 32;
    switch(div_v)
    {
      case 0:
        reg_add = GPIODATA_0;
        break;

      case 1:
        reg_add = GPIODATA_1;
        break;

      case 2:
        reg_add = GPIODATA_2;
        break;

      default:
        return -1;
        break;
    }
  	sprintf(command, "devmem 0x%08x", reg_add);
  	send_command(command, resulte, sizeof(resulte));
  	reg_val = strtoul(resulte + 2, NULL, 16);
    if(val)
    {
        reg_val |= (val << mod_v);
    }
    else
    {
        reg_val &= ~((val | 1) << mod_v);
    }
    sprintf(command, "devmem 0x%x 32 0x%08x", reg_add, reg_val);
    send_command(command, NULL, 0);
    return 1;
}


int GpioIOInitial(int num, int mode, int val)
{
	int mod_v, div_v;
	unsigned long reg_add, reg_val, val_temp;
    char command[64], resulte[32];

	div_v = num / 32;
	mod_v = num % 32;
  //printf("GpioIOInitial : GPIO_Num : %d, div_v : %d, mod_v : %d\n", num, div_v, mod_v);

	if((div_v > 2) || (mod_v > 32)) {
		return ERR_GPIO_RANGE;
	}

	switch(div_v) {
		case 0:
			reg_add = GPIOCTRL_0;
			break;

		case 1:
			reg_add = GPIOCTRL_1;
			break;

		case 2:
			reg_add = GPIOCTRL_2;
			break;
	}
	sprintf(command, "devmem 0x%08x", reg_add);
	send_command(command, resulte, sizeof(resulte));
	reg_val = strtoul(resulte + 2, NULL, 16);
	if(mode) {
		reg_val |= (1 << mod_v);
	}
	else {
		reg_val &= ~(1 << mod_v);
	}
    sprintf(command, "devmem 0x%x 32 0x%08x", reg_add, reg_val);
    send_command(command, NULL, 0);
	sprintf(command, "devmem 0x%08x", reg_add);
	send_command(command, resulte, sizeof(resulte));
	val_temp = strtoul(resulte + 2, NULL, 16);
	if((val_temp & (1 << mod_v)) != (reg_val & (1 << mod_v))) {
    printf("Check Mode Error!!!!!\n val_temp : 0x%x, reg_val : 0x%x\n", val_temp, reg_val);
		return ERR_GPIO_MODE;
	}

	if(mode) {
		switch(div_v) {
			case 0:
				reg_add = GPIODATA_0;
				break;

			case 1:
				reg_add = GPIODATA_1;
				break;

			case 2:
				reg_add = GPIODATA_2;
				break;
		}
		sprintf(command, "devmem 0x%08x", reg_add);
		send_command(command, resulte, sizeof(resulte));
    	reg_val = strtoul(resulte + 2, NULL, 16);
		if(val) {
			reg_val |= (1 << mod_v);
		} else {
			reg_val &= ~(1 << mod_v);
		}
		sprintf(command, "devmem 0x%08x 32 0x%08x", reg_add, reg_val);
		send_command(command, NULL, 0);
		sprintf(command, "devmem 0x%08x", reg_add);
		send_command(command, resulte, sizeof(resulte));
		val_temp = strtoul(resulte + 2, NULL, 16);
  	if((val_temp & (1 << mod_v)) != (reg_val & (1 << mod_v))) {
  		printf("Check Value Error!!!!!\n val_temp : 0x%x, reg_val : 0x%x\n", val_temp, reg_val);
			return ERR_GPIO_VAL;
		}
	}

	return OK_GPIO;
}

void GpioPinMode(void)
{
    int ret;

	ret = GpioIOInitial(GPIO_USB1PWR_NUM, DIRECT_OUT, 1);
	if(ret) {
		printf("Gpio USB1PWR initial faild!!! Error Code : %d\n", ret);
	}

	ret = GpioIOInitial(GPIO_USB2PWR_NUM, DIRECT_OUT, 1);
	if(ret) {
		printf("Gpio USB2PWR initial faild!!! Error Code : %d\n", ret);
	}

	ret = GpioIOInitial(GPIO_WCEN_NUM, DIRECT_OUT, 1);
	if(ret) {
		printf("Gpio WCEN initial faild!!! Error Code : %d\n", ret);
	}

	ret = GpioIOInitial(GPIO_WCRDY_NUM, DIRECT_IN, 0);
	if(ret) {
		printf("Gpio WCRDY initial faild!!! Error Code : %d\n", ret);
	}
}
