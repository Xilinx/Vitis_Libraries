#!/bin/env python

import sys
import xml.etree.ElementTree

def vbnv(root):
	v = root.attrib['{http://www.xilinx.com/Vitis}vendor']
	b = root.attrib['{http://www.xilinx.com/Vitis}library']
	n = root.attrib['{http://www.xilinx.com/Vitis}name']
	r = root.attrib['{http://www.xilinx.com/Vitis}version']

	return ':'.join([v,b,n,r])

def hw(root):
	xpath = './{http://www.xilinx.com/Vitis}hardwarePlatforms/{http://www.xilinx.com/Vitis}hardwarePlatform'
	Vitis = root.find(xpath).attrib

	dir = Vitis['{http://www.xilinx.com/Vitis}path']
	xsa = Vitis['{http://www.xilinx.com/Vitis}name']

	return dir + '/' + xsa

file = sys.argv[1]
mode = sys.argv[2]

tree = xml.etree.ElementTree.parse(file)

root = tree.getroot()

if mode == "xsa":
	print vbnv(root)

if mode == "hw":
	print hw(root)
