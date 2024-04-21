library(readr)
library(latex2exp)

# read the table
scriptsDir="C:/Users/Adam/source/repos/uni/CMP400-Project/scripts/Analysis/"
data <- read_csv(paste0(scriptsDir, "../../data/GTX1050Ti/Sample Latency/StandardMarchLatency.csv"))

# prune rows for a range
dataRange = "D3DVolumeMarch"
data <- data[data$name==dataRange, ]

# convert to milliseconds
millisecs = data$mean_ns / 1000000

#png(file=paste0(scriptsDir, "StandardMarchLatency.png"), 
width = 1024, 
height = 1024, 
pointsize = 24)

# plot the table
table <- plot(data$Samples, millisecs,
col=c("black"),
main = "Latency With Samples",
sub = paste0("(Range = ", dataRange, ")"),
ylab="Average Latency (ms)",
xlab="Sample Count",
pch=3)

# determine regression
y = millisecs
x = data$Samples
regression <- lm(formula=y~x, data=data)
summary(regression)

# display and output summary of the fit
summ = summary(regression)
capture.output(summ, file = paste0(scriptsDir, "StandardMarchLatency.txt"))

# plot predicted line
prediction <- predict(regression)
lines(data$Samples, prediction, col='blue')

# display predicted legend
legend("topleft", 
legend=c(TeX(r'($y\sim k_x\cdot x+k$)', bold=FALSE)), 
col=c("blue"), lty=1, cex=0.75)

#dev.off()

