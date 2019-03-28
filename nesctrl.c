#include <linux/module.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/spi/spi.h>


#define A_VAL ((rxBuffer[0] >> 7) & 1)
#define B_VAL ((rxBuffer[0] >> 6) & 1)
#define SELECT_VAL ((rxBuffer[0] >> 5) & 1)
#define START_VAL ((rxBuffer[0] >> 4) & 1)
#define UP_VAL ((rxBuffer[0] >> 3) & 1)
#define DOWN_VAL ((rxBuffer[0] >> 2) & 1)
#define LEFT_VAL ((rxBuffer[0] >> 1) & 1)
#define RIGHT_VAL (rxBuffer[0] & 1)


MODULE_AUTHOR("JP Lassonde");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("NES controller SPI driver");

struct nesctrl_device {
	struct input_polled_dev *poll_dev; 
	struct spi_device *spi;
	u8 rxBuff[1];
};

static void nesctrl_poll(struct input_polled_dev *poll_dev) 
{
	char rxBuffer[1];
	int ret_val;
	struct nesctrl_device *ctrl = poll_dev->private;
	
	ret_val = spi_read(ctrl->spi, &rxBuffer, 1);
	if (ret_val) {
		dev_err(&ctrl->spi->dev, "spi read error: %d\n", ret_val);
		return;
	}	
	rxBuffer[0] = ~rxBuffer[0];

	input_report_key(poll_dev->input, BTN_DPAD_UP, UP_VAL);
	input_report_key(poll_dev->input, BTN_DPAD_DOWN, DOWN_VAL);
	input_report_key(poll_dev->input, BTN_DPAD_LEFT, LEFT_VAL);
	input_report_key(poll_dev->input, BTN_DPAD_RIGHT, RIGHT_VAL);
	input_report_key(poll_dev->input, BTN_GAMEPAD, A_VAL);
	input_report_key(poll_dev->input, BTN_B, B_VAL);
	input_report_key(poll_dev->input, BTN_START, START_VAL);
	input_report_key(poll_dev->input, BTN_SELECT, SELECT_VAL);

	input_sync(poll_dev->input);
}

static int nesctrl_probe(struct spi_device *spi) 
{
	struct nesctrl_device *nesctrl;
	struct input_polled_dev *poll_dev;
	struct input_dev *input_dev;
	int ret_status;		

	nesctrl = kzalloc(sizeof(*nesctrl), GFP_KERNEL);
	if (!nesctrl) {
		return -ENOMEM;
	}
	
	poll_dev = input_allocate_polled_device();
	if (!poll_dev) {
		kfree(nesctrl);
		return -ENOMEM;
	}
	
	nesctrl->poll_dev = poll_dev;
	nesctrl->spi = spi;
	
	poll_dev->private = nesctrl;
	poll_dev->poll = nesctrl_poll;
	poll_dev->poll_interval = 20; // 50 Hz poll
	
	input_dev = poll_dev->input;
	input_dev->id.bustype = BUS_SPI;
	input_dev->name = "NES Controller";
	
	input_set_capability(input_dev, EV_KEY, BTN_DPAD_UP);
	input_set_capability(input_dev, EV_KEY, BTN_DPAD_DOWN);
	input_set_capability(input_dev, EV_KEY, BTN_DPAD_LEFT);
	input_set_capability(input_dev, EV_KEY, BTN_DPAD_RIGHT);
	input_set_capability(input_dev, EV_KEY, BTN_GAMEPAD);
	input_set_capability(input_dev, EV_KEY, BTN_B);
	input_set_capability(input_dev, EV_KEY, BTN_START);
	input_set_capability(input_dev, EV_KEY, BTN_SELECT);
	
	spi->mode = SPI_MODE_3;
	spi->bits_per_word = 8;
	spi->max_speed_hz = 100000;
	spi_setup(spi);

	ret_status = input_register_polled_device(poll_dev);
	if (ret_status != 0) {
		dev_err(&spi->dev, "polled device could not be registered: %d\n", ret_status);
		input_free_polled_device(poll_dev);
		kfree(nesctrl);
		return ret_status;
	}

	return 0;
}

static const struct spi_device_id nes_id[] = {
	 { "nesctrl", 0}, {}
};

MODULE_DEVICE_TABLE(spi, nes_id);

static struct spi_driver nes_driver = {
	.id_table = nes_id,
	.probe = nesctrl_probe,
	.driver = {
		.name = "nesctrl",
		.owner = THIS_MODULE
	}
};

// replace module_init / module_exit.
module_spi_driver(nes_driver);



