library(readr)
library(latex2exp)

# read the table
scriptsDir="C:/Users/Adam/source/repos/uni/CMP400-Project/scripts/Analysis/"
data <- read_csv(paste0(scriptsDir, "../../data/GTX1050Ti/Sample Convergence/StandardConvergence.csv"))

png(file=paste0(scriptsDir, "StandardConvergence.png"), 
width = 1024, 
height = 1024, 
pointsize = 24)

# plot the table
table <- plot(data$Samples, data$"RMS",
col=c("black"),
main = "Convergence With Samples",
ylab="RMSE to Ground Truth",
xlab="Sample Count",
pch=3)

# determine regression
y=data$"RMS"
x=data$Samples^-1
regression <- lm(formula=y~x, data=data)
summary(regression)

# display and output summary of the fit
summ = summary(regression)
capture.output(summ, file = paste0(scriptsDir, "StandardConvergence.txt"))

# plot predicted line
prediction <- predict(regression)
lines(data$Samples, prediction, col='blue')

# display predicted legend
legend("center", 
legend=c(TeX(r'($y\sim \frac{x_k}{x}+k$)', bold=FALSE)), 
col=c("blue"), lty=1, cex=0.75)

dev.off()

