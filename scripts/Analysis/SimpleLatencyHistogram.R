library(readr)

# read the table
scriptsDir="C:/Users/Adam/source/repos/uni/CMP400-Project/scripts/Analysis/"
data <- read_csv(paste0(scriptsDir, "../../data/GTX1050Ti/Simple Latency/StandardLatency.csv"))

# prune rows for a range
dataRange = "D3DVolumeMarch"
data <- data[data$name==dataRange, ]

# convert to milliseconds
millisecs = data$exec_time_ns / 1000000

#png(file=paste0(scriptsDir, "SimpleStandardLatency.png"), 
width = 1024, 
height = 1024, 
pointsize = 24)

# plot the table
table <- hist(millisecs,
col=c("lightblue"),
main = "Static Latency",
sub = paste0("(Range = ", dataRange, ")"),
ylab="Frame Count",
xlab="Latency (ms)",
breaks = 100)

#dev.off()