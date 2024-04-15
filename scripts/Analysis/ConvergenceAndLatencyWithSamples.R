library(readr)

# read the tables
data1 <- read_csv("C:/Users/Adam/source/builds/uni/CMP400/Release/Convergence.csv")
data2 <- read_csv("C:/Users/Adam/source/builds/uni/CMP400/Release/MarchLatency.csv")

# prune rows for a range
data2 <- data2[data2$name=="VolumeMarch", ]

# plot the table
table <- plot(data1$Samples, data2$max_ns,
col=c("black"),
main = "Convergence",
ylab="Latency * RMS (err ns)",
xlab="Sample Count")

# determine regression
y=data2$min_ns
x=data1$Samples^-1
x2=data1$Samples^-2
regression <- lm(formula=y~x+x2, data=data)
summary(regression)

# plot predicted line
prediction <- predict(regression)
lines(data1$Samples, prediction, col='blue')

