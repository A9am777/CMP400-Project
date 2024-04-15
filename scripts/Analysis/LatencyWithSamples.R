library(readr)

# read the table
data <- read_csv("C:/Users/Adam/source/builds/uni/CMP400/Release/MarchLatency.csv")

# prune rows for a range
data <- data[data$name=="VolumeMarch", ]

# plot the table
table <- plot(data$Samples, data$mean_ns,
col=c("black"),
main = "Convergence",
ylab="Mean Latency (ns)",
xlab="Sample Count")

