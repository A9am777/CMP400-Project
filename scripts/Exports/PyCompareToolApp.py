import os

# Arguments
import argparse
parser=argparse.ArgumentParser(description="Compare images, duh")
parser.add_help=True
parser.add_argument("testImg", help="Path to the test image")
parser.add_argument("groundImg", help="Path to the ground truth image")

args=parser.parse_args()

# Extra imports
import numpy as np

# Compare
import cv2
import image_similarity_measures
from image_similarity_measures.quality_metrics import rmse, psnr, sre

gndImage = cv2.imread(args.groundImg)
testImage = cv2.imread(args.testImg)

rmseImages = rmse(testImage, gndImage)
sreImages = sre(testImage, gndImage)

print("Root Mean Squared: " + str(rmseImages) + ", Signal to Reconstruction: " + str(sreImages))