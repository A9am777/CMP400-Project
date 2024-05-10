library(readr)
library(latex2exp)

scriptsDir="C:/Users/Adam/source/repos/uni/CMP400-Project/scripts/Analysis/"

# read the first table
data1 <- read_csv(paste0(scriptsDir, "../../data/GTX1050Ti/Sample Convergence/StandardConvergence.csv"))

# read the second table
data2 <- read_csv(paste0(scriptsDir, "../../data/GTX1050Ti/Sample Convergence/NoUpscalingConvergence.csv"))

print(data1$RMS)

wilcoxSummary <- wilcox.test(data1$RMS, data2$RMS, paired = TRUE, alternative = "two.sided")
print(wilcoxSummary)
capture.output(wilcoxSummary, file = paste0(scriptsDir, "StandardVSUpscaleConvergence.txt"))