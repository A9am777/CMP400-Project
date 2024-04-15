library(readr)

# read the table
data <- read_csv("C:/Users/Adam/source/builds/uni/CMP400/Release/Spatial.csv")

# plot the table
table <- plot(data$Progress, data$"RMS",
col=c("black"),
main = "Percieved Error With Orbit",
ylab="RMS",
xlab="Location")
