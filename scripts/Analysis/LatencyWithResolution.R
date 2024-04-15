library(readr)

# read the table
data <- read_csv("C:/Users/Adam/source/builds/uni/CMP400/Release/Resolution.csv")

# prune rows for a range
data <- data[data$name=="VolumeMarch", ]

# plot the table
table <- plot(data$Resolution, data$mean_ns,
col=c("black"),
main = "Square Resolution Latency",
ylab="Mean Latency (ns)",
xlab="Resolution Dimension")

# arrows(x0 = data$Resolution, y0 = data$min_ns, x1 = data$Resolution, y1 = data$max_ns, ength = 0.15, code = 3, angle = 90)