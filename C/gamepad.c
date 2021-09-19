#!/bin/bash
# contains and uses this version of Jason White's joystick.c at bottom
# https://gist.github.com/jasonwhite/c5b2048c15993d285130/eb3bcd6e1cd88002d21764b9965d075eceaf4b83
#
# requires SNES joystick connected to Raspberry USB, pigpio library installed
#
g=1500
l=1500
u=600
p=1500
d=100
calib=0

if [ ! -f /var/run/pigpio.pid ]; then sudo pigpiod; sleep 0.5; fi

pigs s 8 $g; pigs s 9 $u; pigs s 10 $l; pigs s 11 $p

while IFS= read -r line; do
    case $line in
        "Axis 0 at (-32767,      0)")
            let p=p+d
        ;;
        "Axis 0 at ( 32767,      0)")
            let p=p-d
        ;;
        "Axis 0 at (     0, -32767)")
            let l=l+d
        ;;
        "Axis 0 at (     0,  32767)")
            let l=l-d
        ;;
        "Button 0 pressed")
            let u=u+d
        ;;
        "Button 2 pressed")
            let u=u-d
        ;;
        "Button 1 pressed")
            let g=g+d
        ;;
        "Button 3 pressed")
            let g=g-d
        ;;
        "Button 4 pressed")
            let d=d/10
            if [ $d -lt 10 ]; then
                d=10
            fi
        ;;
        "Button 5 pressed")
            let d=d*10
            if [ $d -gt 1000 ]; then
                d=1000
            fi
        ;;
        "Button 8 pressed")
            g=2500
            d=100
            calib=1
        ;;
        "Button 9 pressed")
            g=1500
            l=1500
            u=600
            p=1500
            d=100
            calib=0
        ;;
    esac

    if [ $g -lt  550 ]; then g=550; fi
    if [ $g -gt 2000 -a $calib -lt 1 ]; then g=2000; fi
    if [ $u -lt  600 ]; then u=600; fi
    if [ $u -gt 2250 ]; then u=2250; fi
    if [ $l -lt  500 ]; then l=500; fi
    if [ $l -gt 2500 ]; then l=2500; fi
    if [ $p -lt  500 ]; then p=500; fi
    if [ $p -gt 2500 ]; then p=2500; fi

echo $g $u $l $p $d $calib

    pigs s 8 $g; pigs s 9 $u; pigs s 10 $l; pigs s 11 $p

done < <(./memrun <(gcc -o /dev/fd/1 -x c <(sed -n "/^\/\*\*$/,\$p" $0)))

exit
/**
 * Author: Jason White
 *
 * Description:
 * Reads joystick/gamepad events and displays them.
 *
 * Compile:
 * gcc joystick.c -o joystick
 *
 * Run:
 * ./joystick [/dev/input/jsX]
 *
 * See also:
 * https://www.kernel.org/doc/Documentation/input/joystick-api.txt
 */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>


/**
 * Reads a joystick event from the joystick device.
 *
 * Returns 0 on success. Otherwise -1 is returned.
 */
int read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == sizeof(*event))
        return 0;

    /* Error, could not read full event. */
    return -1;
}

/**
 * Returns the number of axes on the controller or 0 if an error occurs.
 */
size_t get_axis_count(int fd)
{
    __u8 axes;

    if (ioctl(fd, JSIOCGAXES, &axes) == -1)
        return 0;

    return axes;
}

/**
 * Returns the number of buttons on the controller or 0 if an error occurs.
 */
size_t get_button_count(int fd)
{
    __u8 buttons;
    if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1)
        return 0;

    return buttons;
}

/**
 * Current state of an axis.
 */
struct axis_state {
    short x, y;
};

/**
 * Keeps track of the current axis state.
 *
 * NOTE: This function assumes that axes are numbered starting from 0, and that
 * the X axis is an even number, and the Y axis is an odd number. However, this
 * is usually a safe assumption.
 *
 * Returns the axis that the event indicated.
 */
size_t get_axis_state(struct js_event *event, struct axis_state axes[3])
{
    size_t axis = event->number / 2;

    if (axis < 3)
    {
        if (event->number % 2 == 0)
            axes[axis].x = event->value;
        else
            axes[axis].y = event->value;
    }

    return axis;
}

int main(int argc, char *argv[])
{
    const char *device;
    int js;
    struct js_event event;
    struct axis_state axes[3] = {0};
    size_t axis;

    if (argc > 1)
        device = argv[1];
    else
        device = "/dev/input/js0";

    js = open(device, O_RDONLY);

    if (js == -1)
        perror("Could not open joystick");

    /* This loop will exit if the controller is unplugged. */
    while (read_event(js, &event) == 0)
    {
        switch (event.type)
        {
            case JS_EVENT_BUTTON:
                printf("Button %u %s\n", event.number, event.value ? "pressed" : "released");
                break;
            case JS_EVENT_AXIS:
                axis = get_axis_state(&event, axes);
                if (axis < 3)
                    printf("Axis %zu at (%6d, %6d)\n", axis, axes[axis].x, axes[axis].y);
                break;
            default:
                /* Ignore init events. */
                break;
        }
        
        fflush(stdout);
    }

    close(js);
    return 0;
}
