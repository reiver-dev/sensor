/*
 * hw_address.hpp
 *
 *  Created on: 11.12.2012
 *      Author: reiver
 */

#ifndef HW_ADDRESS_HPP_
#define HW_ADDRESS_HPP_

#include <net/ethernet.h>

class HwAddress {
public:

	HwAddress(struct ether_addr hwaddr) : address(hwaddr) {/* */}

	HwAddress(const HwAddress &other) : address(other.address) {/* */}

	void operator==(const HwAddress &other) {
		address = other.address;
	}

private:

	struct ether_addr address;

};

#endif /* HW_ADDRESS_HPP_ */
