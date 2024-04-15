library(readr)

# read the table
data <- read_csv("C:/Users/Adam/source/builds/uni/CMP400/Release/Convergence.csv")

# plot the table
table <- plot(data$Samples, data$"RMS",
col=c("black"),
main = "Convergence",
ylab="RMS",
xlab="Sample Count")

# determine regression
y=data$"RMS"
x=data$Samples^-1
x2=data$Samples^-2
regression <- lm(formula=y~x+x2, data=data)
summary(regression)

# plot predicted line
prediction <- predict(regression)
lines(data$Samples, prediction, col='blue')

