import os

# Arguments
import argparse
parser=argparse.ArgumentParser(description="Denoise images, duh")
parser.add_help=True
parser.add_argument("srcImg", help="Path to the noisy image")
parser.add_argument("filterSize", help="Size of the reconstruction filter")
parser.add_argument("outImg", help="Path to export the denoised image to")

args=parser.parse_args()

# Extra imports
import numpy as np

# Denoise
import cv2

srcImage = cv2.imread(args.srcImg)
wienerImage = cv2.medianBlur(srcImage, int(args.filterSize))
cv2.imwrite(args.outImg, wienerImage)